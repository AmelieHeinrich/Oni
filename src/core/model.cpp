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

    // Optimization
    meshopt_optimizeVertexCache(indices.data(), indices.data(), indices.size(), vertices.size());
    meshopt_optimizeOverdraw(indices.data(), indices.data(), indices.size(), &vertices[0].Position.x, vertices.size(), sizeof(Vertex), 1.05f);

    // Generate meshlets
    std::vector<Meshlet> meshlets;
    std::vector<uint32_t> meshletVertices;
    std::vector<MeshletTriangle> meshletTriangles;

    const size_t maxVertices = MAX_MESHLET_VERTICES;
    const size_t maxTriangles = MAX_MESHLET_TRIANGLES;
    const size_t maxMeshlets = meshopt_buildMeshletsBound(indices.size(), maxVertices, maxTriangles);

    meshlets.resize(maxMeshlets);
    meshletVertices.resize(maxMeshlets * maxVertices);
    std::vector<unsigned char> tempTris(maxMeshlets * maxTriangles * 3);
    std::vector<meshopt_Meshlet> tempMeshlets(maxMeshlets);

    size_t meshlet_count = meshopt_buildMeshlets(tempMeshlets.data(),
                                                 meshletVertices.data(),
                                                 tempTris.data(),
                                                 indices.data(),
                                                 indices.size(),
                                                 &vertices[0].Position.x,
                                                 vertices.size(),
                                                 sizeof(Vertex),
                                                 maxVertices,
                                                 maxTriangles,
                                                 0);

    // Trimming meshlets
    const meshopt_Meshlet& last = tempMeshlets[meshlet_count - 1];
	tempTris.resize(last.triangle_offset + ((last.triangle_count * 3 + 3) & ~3));
	tempMeshlets.resize(meshlet_count);

    meshlets.resize(meshlet_count);
    meshletVertices.resize(last.vertex_offset + last.vertex_count);
	meshletTriangles.resize(tempTris.size() / 3);

    uint32_t triangleOffset = 0;
    for (size_t i = 0; i < meshlet_count; ++i) {
        const meshopt_Meshlet& meshlet = tempMeshlets[i];
    
        // TODO: Generate bounding box and sphere

        unsigned char* pSourceTriangles = tempTris.data() + meshlet.triangle_offset;
        meshopt_optimizeMeshlet(&meshletVertices[meshlet.vertex_offset], pSourceTriangles, meshlet.triangle_count, meshlet.vertex_count);

        for (uint32_t triIdx = 0; triIdx < meshlet.triangle_count; ++triIdx) {
			MeshletTriangle& tri = meshletTriangles[triIdx + triangleOffset];
			tri.V0 = *pSourceTriangles++;
			tri.V1 = *pSourceTriangles++;
			tri.V2 = *pSourceTriangles++;
		}

		Meshlet& outMeshlet = meshlets[i];
		outMeshlet.PrimCount = meshlet.triangle_count;
		outMeshlet.PrimOffset = triangleOffset;
		outMeshlet.VertCount = meshlet.vertex_count;
		outMeshlet.VertOffset = meshlet.vertex_offset;
		triangleOffset += meshlet.triangle_count;
    }
    meshletTriangles.resize(triangleOffset);
    
    out.MeshletCount = meshlet_count;

    // GPU UPLOADING

    out.VertexBuffer = context->CreateBuffer(out.VertexCount * sizeof(Vertex), sizeof(Vertex), BufferType::Vertex, false, "Vertex Buffer");
    out.VertexBuffer->BuildShaderResource();

    out.IndexBuffer = context->CreateBuffer(out.IndexCount * sizeof(uint32_t), sizeof(uint32_t), BufferType::Index, false, "Index Buffer");
    out.IndexBuffer->BuildShaderResource();

    out.MeshletBuffer = context->CreateBuffer(out.MeshletCount * sizeof(Meshlet), sizeof(Meshlet), BufferType::Storage, false, "Meshlet Buffer");
    out.MeshletBuffer->BuildShaderResource();

    out.MeshletTriangles = context->CreateBuffer(meshletTriangles.size() * sizeof(MeshletTriangle), sizeof(MeshletTriangle), BufferType::Storage, false, "Meshlet Triangle Buffer");
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

    uploader.CopyHostToDeviceLocal(vertices.data(), vertices.size() * sizeof(Vertex), out.VertexBuffer);
    uploader.CopyHostToDeviceLocal(indices.data(), indices.size() * sizeof(uint32_t), out.IndexBuffer);
    uploader.CopyHostToDeviceLocal(meshlets.data(), meshlets.size() * sizeof(Meshlet), out.MeshletBuffer);
    uploader.CopyHostToDeviceLocal(meshletTriangles.data(), meshletTriangles.size() * sizeof(MeshletTriangle), out.MeshletTriangles);
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
