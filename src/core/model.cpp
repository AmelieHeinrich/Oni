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

void Model::ProcessPrimitive(RenderContext::Ptr context, cgltf_primitive *primitive, glm::mat4 transform, std::string name)
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

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    std::vector<glm::vec3> positions;

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

        positions.push_back(vertex.Position);
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

    // Optimize
    //std::vector<uint32_t> remap_table(vertices.size());
	//uint32_t unique_vertices = meshopt_generateVertexRemap(remap_table.data(), indices.data(), indices.size(), vertices.data(), vertices.size(), sizeof(Vertex));
//
	//vertices.resize(unique_vertices);
	//
    //meshopt_remapIndexBuffer(indices.data(), indices.data(), indices.size(), remap_table.data());
	//meshopt_remapVertexBuffer(vertices.data(), vertices.data(), vertices.size(), sizeof(Vertex), remap_table.data());
    //
	//meshopt_optimizeVertexCache(indices.data(), indices.data(), indices.size(), vertices.size());
	//meshopt_optimizeOverdraw(indices.data(), indices.data(), indices.size(), (float*)vertices.data(), vertices.size(), sizeof(Vertex), 1.05f);
	//meshopt_optimizeVertexFetch(vertices.data(), indices.data(), indices.size(), vertices.data(), vertices.size(), sizeof(Vertex));

    // Generate meshlets
    const size_t maxVertices = MAX_MESHLET_VERTICES;
    const size_t maxTriangles = MAX_MESHLET_TRIANGLES;
    const float coneWeight = 0.0f;

    const size_t maxMeshlets = meshopt_buildMeshletsBound(indices.size(), maxVertices, maxTriangles);

    std::vector<meshopt_Meshlet> meshlets(maxMeshlets);
    std::vector<uint32_t> meshletVertices(maxMeshlets * maxVertices);
    std::vector<uint8_t> meshletTriangles(maxMeshlets * maxTriangles * 3);

    size_t meshletCount = meshopt_buildMeshlets(
            meshlets.data(),
            meshletVertices.data(),
            meshletTriangles.data(),
            reinterpret_cast<const uint32_t*>(indices.data()),
            indices.size(),
            reinterpret_cast<const float*>(positions.data()),
            positions.size(),
            sizeof(glm::vec3),
            maxVertices,
            maxTriangles,
            coneWeight);

    meshlets.resize(meshletCount);
    meshletVertices.resize(meshletCount * 64);
    meshletTriangles.resize(meshletCount * 124 * 3);

    // Repack triangles from 3 consecutive byes to 4-byte uint32_t to
    // make it easier to unpack on the GPU.
    //
    std::vector<uint32_t> meshletTrianglesU32;
    for (auto& m : meshlets) {
        // Save triangle offset for current meshlet
        uint32_t triangleOffset = static_cast<uint32_t>(meshletTrianglesU32.size());
        
        // Bugs out for some reason
        // meshopt_optimizeMeshlet(meshletVertices.data(), meshletTriangles.data(), maxTriangles, maxVertices);

        // Repack to uint32_t
        for (uint32_t i = 0; i < m.triangle_count; ++i) {
            uint32_t i0 = 3 * i + 0 + m.triangle_offset;
            uint32_t i1 = 3 * i + 1 + m.triangle_offset;
            uint32_t i2 = 3 * i + 2 + m.triangle_offset;

            uint8_t  vIdx0  = meshletTriangles[i0];
            uint8_t  vIdx1  = meshletTriangles[i1];
            uint8_t  vIdx2  = meshletTriangles[i2];
            uint32_t packed = ((static_cast<uint32_t>(vIdx0) & 0xFF) << 0) |
                              ((static_cast<uint32_t>(vIdx1) & 0xFF) << 8) |
                              ((static_cast<uint32_t>(vIdx2) & 0xFF) << 16);
            meshletTrianglesU32.push_back(packed);
        }

        // Update triangle offset for current meshlet
        m.triangle_offset = triangleOffset;
    }

    out.MeshletCount = meshlets.size();

    // GPU UPLOADING

    out.VertexBuffer = context->CreateBuffer(out.VertexCount * sizeof(Vertex), sizeof(Vertex), BufferType::Vertex, false, "Vertex Buffer");
    out.VertexBuffer->BuildShaderResource();

    out.IndexBuffer = context->CreateBuffer(out.IndexCount * sizeof(uint32_t), sizeof(uint32_t), BufferType::Index, false, "Index Buffer");
    out.IndexBuffer->BuildShaderResource();

    out.MeshletBuffer = context->CreateBuffer(out.MeshletCount * sizeof(Meshlet), sizeof(Meshlet), BufferType::Storage, false, "Meshlet Buffer");
    out.MeshletBuffer->BuildShaderResource();

    out.MeshletTriangles = context->CreateBuffer(meshletTrianglesU32.size() * sizeof(uint32_t), sizeof(uint32_t), BufferType::Storage, false, "Meshlet Triangle Buffer");
    out.MeshletTriangles->BuildShaderResource();

    for (int i = 0; i < 3; i++) {
        out.ModelBuffer[i] = context->CreateBuffer(256, 0, BufferType::Constant, false, "Model Buffer");
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
    uploader.CopyHostToDeviceLocal(meshlets.data(), meshlets.size() * sizeof(Meshlet), out.MeshletBuffer);
    uploader.CopyHostToDeviceLocal(meshletTrianglesU32.data(), meshletTrianglesU32.size() * sizeof(MeshletTriangle), out.MeshletTriangles);
    
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
    temp.Transform = out.Transform;
    temp.PrevTransform = out.Transform;

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

void Model::ProcessNode(RenderContext::Ptr context, cgltf_node *node, glm::mat4 transform)
{
    glm::mat4 localTransform = transform;
    glm::mat4 translationMatrix(1.0f);
    glm::mat4 rotationMatrix(1.0f);
    glm::mat4 scaleMatrix(1.0f);

    if (node->has_translation) {
        translationMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(node->translation[0], node->translation[1], node->translation[2]));
    }
    if (node->has_rotation) {
        rotationMatrix = glm::mat4_cast(glm::quat(node->rotation[3], node->rotation[0], node->rotation[1], node->rotation[2]));
    }
    if (node->has_scale) {
        scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(node->scale[0], node->scale[1], node->scale[2]));
    }

    if (node->has_matrix) {
        localTransform *= glm::make_mat4(node->matrix);
    } else {
        localTransform *= translationMatrix * rotationMatrix * scaleMatrix;
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
        ProcessNode(renderContext, scene->nodes[i], glm::mat4(1.0f));
    }
    Logger::Info("[CGLTF] Successfully loaded model at path %s", path.c_str());
    cgltf_free(data);
}

void Model::ApplyTransform(glm::mat4 transform)
{
    for (auto& primitive : Primitives) {
        primitive.Transform *= transform;
    }
}
