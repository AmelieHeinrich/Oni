/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-31 13:34:44
 */

#include "model.hpp"

#include "core/bitmap.hpp"
#include "core/log.hpp"
#include "core/texture_compressor.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <meshopt/meshoptimizer.h>

#undef min
#undef max

void Model::ProcessPrimitive(RenderContext::Ptr context, cgltf_primitive *primitive, Transform transform, std::string name)
{
    // Start
    if (primitive->type != cgltf_primitive_type_triangles) {
        Logger::Warn("[CGLTF] GLTF primitive isn't a triangle list, discarding.");
        return;
    }

    Primitive out;
    out.Transform = transform;
    if (name.empty()) {
        out.Name = "GLTF Node";
    } else {
        out.Name = name;
    }

    // Get attributes
    cgltf_attribute* pos_attribute = nullptr;
    cgltf_attribute* uv_attribute = nullptr;
    cgltf_attribute* norm_attribute = nullptr;

    for (int i = 0; i < primitive->attributes_count; i++) {
        if (!strcmp(primitive->attributes[i].name, "POSITION")) {
            pos_attribute = &primitive->attributes[i];
        }
        if (!strcmp(primitive->attributes[i].name, "TEXCOORD_0")) {
            uv_attribute = &primitive->attributes[i];
        }
        if (!strcmp(primitive->attributes[i].name, "NORMAL")) {
            norm_attribute = &primitive->attributes[i];
        }
    }
    if (!pos_attribute || !uv_attribute || !norm_attribute) {
        Logger::Warn("[CGLTF] Didn't find all GLTF attributes, discarding.");
        return;
    }

    // Load vertices
    int vertexCount = pos_attribute->data->count;
    int indexCount = primitive->indices->count;

    std::vector<Vertex> vertices = {};
    std::vector<uint32_t> indices = {};

    for (int i = 0; i < vertexCount; i++) {
        Vertex vertex;

        if (!cgltf_accessor_read_float(pos_attribute->data, i, glm::value_ptr(vertex.Position), 4)) {
            Logger::Warn("[CGLTF] Failed to read all position attributes!");
        }
        if (!cgltf_accessor_read_float(uv_attribute->data, i, glm::value_ptr(vertex.UV), 4)) {
            Logger::Warn("[CGLTF] Failed to read all UV attributes!");
        }
        if (!cgltf_accessor_read_float(norm_attribute->data, i, glm::value_ptr(vertex.Normals), 4)) {
            Logger::Warn("[CGLTF] Failed to read all normal attributes!");
        }

        vertices.push_back(vertex);
    }

    for (int i = 0; i < indexCount; i++) {
        indices.push_back(cgltf_accessor_read_index(primitive->indices, i));
    }

    out.VertexCount = vertices.size();
    out.IndexCount = indices.size();

    out.BoundingBox.Min = glm::vec3(FLT_MAX, FLT_MAX, FLT_MAX);
    out.BoundingBox.Max = glm::vec3(FLT_MIN, FLT_MIN, FLT_MIN);

    for (uint32_t j = 0; j < vertices.size(); ++j) {
        out.BoundingBox.Min = glm::min(out.BoundingBox.Min, vertices[j].Position);
        out.BoundingBox.Max = glm::max(out.BoundingBox.Max, vertices[j].Position);
    }

    out.BoundingBox.Center = (out.BoundingBox.Min + out.BoundingBox.Max) / glm::vec3(2);
    out.BoundingBox.Extent = (out.BoundingBox.Max - out.BoundingBox.Min);

    // Generate meshlets
    std::vector<meshopt_Meshlet> meshlets = {};
    std::vector<uint32_t> meshletVertices = {};
    std::vector<uint8_t> meshletTriangles = {};
    std::vector<MeshletBounds> meshletBounds = {};

    const size_t kMaxTriangles = MAX_MESHLET_TRIANGLES;
    const size_t kMaxVertices = MAX_MESHLET_VERTICES;
    const float kConeWeight = 0.0f;

    size_t maxMeshlets = meshopt_buildMeshletsBound(indices.size(), kMaxVertices, kMaxTriangles);

    meshlets.resize(maxMeshlets);
    meshletVertices.resize(maxMeshlets * kMaxVertices);
    meshletTriangles.resize(maxMeshlets * kMaxTriangles * 3);

    size_t meshletCount = meshopt_buildMeshlets(
            meshlets.data(),
            meshletVertices.data(),
            meshletTriangles.data(),
            reinterpret_cast<const uint32_t*>(indices.data()),
            indices.size(),
            reinterpret_cast<const float*>(vertices.data()),
            vertices.size(),
            sizeof(Vertex),
            kMaxVertices,
            kMaxTriangles,
            kConeWeight);

    const meshopt_Meshlet& last = meshlets[meshletCount - 1];
    meshletVertices.resize(last.vertex_offset + last.vertex_count);
    meshletTriangles.resize(last.triangle_offset + ((last.triangle_count * 3 + 3) & ~3));
    meshlets.resize(meshletCount);

    for (auto& m : meshlets) {
        meshopt_optimizeMeshlet(&meshletVertices[m.vertex_offset], &meshletTriangles[m.triangle_offset], m.triangle_count, m.vertex_count);
    
        // Generate bounds
        meshopt_Bounds meshopt_bounds = meshopt_computeMeshletBounds(&meshletVertices[m.vertex_offset], &meshletTriangles[m.triangle_offset],
                                                                     m.triangle_count, &vertices[0].Position.x, vertices.size(), sizeof(Vertex));
        
        MeshletBounds bounds;
        memcpy(glm::value_ptr(bounds.center), meshopt_bounds.center, sizeof(float) * 3);
        memcpy(glm::value_ptr(bounds.cone_apex), meshopt_bounds.cone_apex, sizeof(float) * 3);
        memcpy(glm::value_ptr(bounds.cone_axis), meshopt_bounds.cone_axis, sizeof(float) * 3);

        bounds.radius = meshopt_bounds.radius;
        bounds.cone_cutoff = meshopt_bounds.cone_cutoff;
        meshletBounds.push_back(bounds);
    }

    // PUSH
    std::vector<uint32_t> meshletPrimitives;
    for (auto& val : meshletTriangles) {
        meshletPrimitives.push_back(static_cast<uint32_t>(val));
    }

    out.MeshletCount = meshlets.size();

    // GPU UPLOADING

    out.VertexBuffer = context->CreateBuffer(vertices.size() * sizeof(Vertex), sizeof(Vertex), BufferType::Vertex, false, "Vertex Buffer");
    out.VertexBuffer->BuildShaderResource();

    out.IndexBuffer = context->CreateBuffer(indices.size() * sizeof(uint32_t), sizeof(uint32_t), BufferType::Index, false, "Index Buffer");
    out.IndexBuffer->BuildShaderResource();

    out.MeshletBuffer = context->CreateBuffer(meshlets.size() * sizeof(meshopt_Meshlet), sizeof(meshopt_Meshlet), BufferType::Storage, false, "Meshlet Buffer");
    out.MeshletBuffer->BuildShaderResource();

    out.MeshletVertices = context->CreateBuffer(meshletVertices.size() * sizeof(uint32_t), sizeof(uint32_t), BufferType::Storage, false, "Meshlet Vertices");
    out.MeshletVertices->BuildShaderResource();

    out.MeshletTriangles = context->CreateBuffer(meshletPrimitives.size() * sizeof(uint32_t), sizeof(uint32_t), BufferType::Storage, false, "Meshlet Triangle Buffer");
    out.MeshletTriangles->BuildShaderResource();

    out.MeshletBounds = context->CreateBuffer(meshletBounds.size() * sizeof(MeshletBounds), sizeof(MeshletBounds), BufferType::Storage, false, "Meshlet Bounds Buffer");
    out.MeshletBounds->BuildShaderResource();

    if (context->GetDevice()->GetFeatures().Raytracing) {
        // Build BLAS
        out.BottomLevelAS = context->CreateBLAS(out.VertexBuffer, out.IndexBuffer, out.VertexCount, out.IndexCount, "Bottom Level Acceleration Structure");
    }

    for (int i = 0; i < 3; i++) {
        out.ModelBuffer[i] = context->CreateBuffer(512, 0, BufferType::Constant, false, "Model Buffer");
        out.ModelBuffer[i]->BuildConstantBuffer();
    }

    // MATERIAL
    cgltf_material* material = primitive->material;
    Material meshMaterial = {};
    out.MaterialIndex = Materials.size();

    TextureFile albedoImage = {};
    TextureFile normalImage = {};
    TextureFile pbrImage = {};
    TextureFile emissiveImage = {};
    TextureFile aoImage = {};

    glm::vec3 flatColor(1.0f, 1.0f, 1.0f);
    meshMaterial.FlatColor = glm::vec3(flatColor.r, flatColor.g, flatColor.b);

    Uploader uploader = context->CreateUploader();

    uploader.CopyHostToDeviceLocal(vertices.data(), vertices.size() * sizeof(Vertex), out.VertexBuffer);
    uploader.CopyHostToDeviceLocal(indices.data(), indices.size() * sizeof(uint32_t), out.IndexBuffer);
    uploader.CopyHostToDeviceLocal(meshlets.data(), meshlets.size() * sizeof(meshopt_Meshlet), out.MeshletBuffer);
    uploader.CopyHostToDeviceLocal(meshletVertices.data(), meshletVertices.size() * sizeof(uint32_t), out.MeshletVertices);
    uploader.CopyHostToDeviceLocal(meshletPrimitives.data(), meshletPrimitives.size() * sizeof(uint32_t), out.MeshletTriangles);
    uploader.CopyHostToDeviceLocal(meshletBounds.data(), meshletBounds.size() * sizeof(MeshletBounds), out.MeshletBounds);
    uploader.BuildBLAS(out.BottomLevelAS);

    // ALBEDO TEXTURE
    {
        if (material->pbr_metallic_roughness.base_color_texture.texture) {
            std::string texturePath = Directory + '/' + std::string(material->pbr_metallic_roughness.base_color_texture.texture->image->uri);
            meshMaterial.AlbedoPath = texturePath;
            meshMaterial.HasAlbedo = true;
            if (TextureCache.count(texturePath) != 0)  {
                meshMaterial.AlbedoTexture = TextureCache[texturePath];
            } else {
                albedoImage.Load(TextureCompressor::GetCachedPath(texturePath));

                meshMaterial.AlbedoTexture = context->CreateTexture(albedoImage.Width(), albedoImage.Height(), albedoImage.Format(), TextureUsage::ShaderResource, true, meshMaterial.AlbedoPath);
                meshMaterial.AlbedoTexture->BuildShaderResource();

                uploader.CopyHostToDeviceCompressedTexture(&albedoImage, meshMaterial.AlbedoTexture);
                TextureCache[texturePath] = meshMaterial.AlbedoTexture;
            }
        }
    }
    
    // NORMAL TEXTURE
    {
        if (material->normal_texture.texture) {
            std::string texturePath = Directory + '/' + std::string(material->normal_texture.texture->image->uri);
            meshMaterial.NormalPath = texturePath;
            meshMaterial.HasNormal = true;
            if (TextureCache.count(texturePath) != 0) {
                meshMaterial.NormalTexture = TextureCache[texturePath];
            } else {
                std::replace(texturePath.begin(), texturePath.end(), '\\', '/');
                normalImage.Load(TextureCompressor::GetCachedPath(texturePath));
                meshMaterial.NormalTexture = context->CreateTexture(normalImage.Width(), normalImage.Height(), normalImage.Format(), TextureUsage::ShaderResource, true, meshMaterial.NormalPath);
                meshMaterial.NormalTexture->BuildShaderResource();

                uploader.CopyHostToDeviceCompressedTexture(&normalImage, meshMaterial.NormalTexture);
                TextureCache[texturePath] = meshMaterial.NormalTexture;
            }
        }
    }
    
    // PBR TEXTURE
    {
        if (material->pbr_metallic_roughness.metallic_roughness_texture.texture) {
            std::string texturePath = Directory + '/' + std::string(material->pbr_metallic_roughness.metallic_roughness_texture.texture->image->uri);
            meshMaterial.MetallicRoughnessPath = texturePath;
            meshMaterial.HasMetallicRoughness = true;
            if (TextureCache.count(texturePath) != 0) {
                meshMaterial.PBRTexture = TextureCache[texturePath];
            } else{
                std::replace(texturePath.begin(), texturePath.end(), '\\', '/');
                pbrImage.Load(TextureCompressor::GetCachedPath(texturePath));
                meshMaterial.PBRTexture = context->CreateTexture(pbrImage.Width(), pbrImage.Height(), pbrImage.Format(), TextureUsage::ShaderResource, true, meshMaterial.MetallicRoughnessPath);
                meshMaterial.PBRTexture->BuildShaderResource();

                uploader.CopyHostToDeviceCompressedTexture(&pbrImage, meshMaterial.PBRTexture);
                TextureCache[texturePath] = meshMaterial.PBRTexture;
            }
        } else if (material->specular.specular_texture.texture) {
            std::string texturePath = Directory + '/' + std::string(material->specular.specular_texture.texture->image->uri);
            meshMaterial.MetallicRoughnessPath = texturePath;
            meshMaterial.HasMetallicRoughness = true;
            if (TextureCache.count(texturePath) != 0) {
                meshMaterial.PBRTexture = TextureCache[texturePath];
            } else{
                std::replace(texturePath.begin(), texturePath.end(), '\\', '/');
                pbrImage.Load(TextureCompressor::GetCachedPath(texturePath));
                meshMaterial.PBRTexture = context->CreateTexture(pbrImage.Width(), pbrImage.Height(), pbrImage.Format(), TextureUsage::ShaderResource, true, meshMaterial.MetallicRoughnessPath);
                meshMaterial.PBRTexture->BuildShaderResource();

                uploader.CopyHostToDeviceCompressedTexture(&pbrImage, meshMaterial.PBRTexture);
                TextureCache[texturePath] = meshMaterial.PBRTexture;
            }
        }
    }
    
    // EMISSIVE TEXTURE
    {
        if (material->emissive_texture.texture) {
            std::string texturePath = Directory + '/' + std::string(material->emissive_texture.texture->image->uri);
            meshMaterial.EmissivePath = texturePath;
            meshMaterial.HasEmissive = true;
            if (TextureCache.count(meshMaterial.EmissivePath) != 0) {
                meshMaterial.EmissiveTexture = TextureCache[texturePath];
            } else {
                std::replace(texturePath.begin(), texturePath.end(), '\\', '/');
                emissiveImage.Load(TextureCompressor::GetCachedPath(texturePath));
                meshMaterial.EmissiveTexture = context->CreateTexture(emissiveImage.Width(), emissiveImage.Height(), emissiveImage.Format(), TextureUsage::ShaderResource, true, meshMaterial.EmissivePath);
                meshMaterial.EmissiveTexture->BuildShaderResource();

                uploader.CopyHostToDeviceCompressedTexture(&emissiveImage, meshMaterial.EmissiveTexture);
                TextureCache[texturePath] = meshMaterial.EmissiveTexture;
            }
        }
    }
    
    // AO TEXTURE
    {
        if (material->occlusion_texture.texture) {
            std::string texturePath = Directory + '/' + std::string(material->occlusion_texture.texture->image->uri);
            meshMaterial.AOPath = texturePath;
            meshMaterial.HasAO = true;
            if (TextureCache.count(meshMaterial.AOPath) != 0) {
                meshMaterial.AOTexture = TextureCache[meshMaterial.AOPath];
            } else {
                std::replace(texturePath.begin(), texturePath.end(), '\\', '/');
                aoImage.Load(TextureCompressor::GetCachedPath(texturePath));
                meshMaterial.AOTexture = context->CreateTexture(aoImage.Width(), aoImage.Height(), aoImage.Format(), TextureUsage::ShaderResource, true, meshMaterial.AOPath);
                meshMaterial.AOTexture->BuildShaderResource();

                uploader.CopyHostToDeviceCompressedTexture(&aoImage, meshMaterial.AOTexture);
                TextureCache[texturePath] = meshMaterial.AOTexture;
            }
        }
    }

    context->FlushUploader(uploader);

    struct ModelData {
        glm::mat4 Camera;
        glm::mat4 PrevCamera;
        glm::mat4 Transform;
        glm::mat4 PrevTransform;
    };
    ModelData temp;
    temp.Camera = glm::mat4(1.0f);
    temp.PrevCamera = glm::mat4(1.0f);
    temp.Transform = out.Transform.Matrix;
    temp.PrevTransform = out.Transform.Matrix;

    VertexCount += out.VertexCount;
    IndexCount += out.IndexCount;
    MeshletCount += out.MeshletCount;

    Materials.push_back(meshMaterial);

    for (int i = 0; i < 3; i++) {
        void *pData;
        out.ModelBuffer[i]->Map(0, 0, &pData);
        memcpy(pData, &temp, sizeof(ModelData));
        out.ModelBuffer[i]->Unmap(0, 0);
    }

    Primitives.push_back(out);
}

void Model::ProcessNode(RenderContext::Ptr context, cgltf_node *node, Transform transform)
{
    Transform localTransform = transform;
    glm::mat4 translationMatrix(1.0f);
    glm::mat4 rotationMatrix(1.0f);
    glm::mat4 scaleMatrix(1.0f);

    if (node->has_translation) {
        glm::vec3 translation = glm::vec3(node->translation[0], node->translation[1], node->translation[2]);
        localTransform.Position = translation;
        translationMatrix = glm::translate(glm::mat4(1.0f), translation);
    }
    if (node->has_rotation) {
        // TODO: transform quaternion
        rotationMatrix = glm::mat4_cast(glm::quat(node->rotation[3], node->rotation[0], node->rotation[1], node->rotation[2]));
    }
    if (node->has_scale) {
        glm::vec3 scale = glm::vec3(node->scale[0], node->scale[1], node->scale[2]);
        localTransform.Scale = scale;
        scaleMatrix = glm::scale(glm::mat4(1.0f), scale);
    }

    if (node->has_matrix) {
        localTransform.Matrix *= glm::make_mat4(node->matrix);
    } else {
        localTransform.Matrix *= translationMatrix * rotationMatrix * scaleMatrix;
    }

    if (node->mesh) {
        for (int i = 0; i < node->mesh->primitives_count; i++) {
            std::string name = "Node";
            if (node->name) {
                name = node->name;
            }
            ProcessPrimitive(context, &node->mesh->primitives[i], localTransform, name);
        }
    }

    for (int i = 0; i < node->children_count; i++) {
        ProcessNode(context, node->children[i], localTransform);
    }
}

void Model::Load(RenderContext::Ptr renderContext, const std::string& path)
{
    Name = path;

    cgltf_options options = {};
    cgltf_data* data = nullptr;

    if (cgltf_parse_file(&options, path.c_str(), &data) != cgltf_result_success) {
        Logger::Error("[CGLTF] Failed to parse GLTF %s", path.c_str());
    }
    if (cgltf_load_buffers(&options, data, path.c_str()) != cgltf_result_success) {
        Logger::Error("[CGLTF] Failed to load buffers %s", path.c_str());
    }
    cgltf_scene* scene = data->scene;

    Directory = path.substr(0, path.find_last_of('/'));
    for (int i = 0; i < scene->nodes_count; i++) {
        ProcessNode(renderContext, scene->nodes[i], Transform());
    }
    Logger::Info("[CGLTF] Successfully loaded model at path %s", path.c_str());
    cgltf_free(data);
}

void Model::ApplyTransform(glm::mat4 transform)
{
    for (auto& primitive : Primitives) {
        primitive.Transform.Matrix *= transform;
    }
}
