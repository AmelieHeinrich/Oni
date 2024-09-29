/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-31 13:34:44
 */

#include "model.hpp"

#include "core/bitmap.hpp"
#include "core/log.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

void Model::ProcessPrimitive(RenderContext::Ptr context, cgltf_primitive *primitive, glm::mat4 transform, std::string name)
{
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

    // Geometry
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

        // Pre-transform vertices
        vertex.UV = glm::vec2(vertex.UV.x, 1.0 - vertex.UV.y);

        vertices.push_back(vertex);
    }

    for (int i = 0; i < indexCount; i++) {
        indices.push_back(cgltf_accessor_read_index(primitive->indices, i));
    }

    out.VertexCount = vertices.size();
    out.IndexCount = indices.size();

    out.VertexBuffer = context->CreateBuffer(out.VertexCount * sizeof(Vertex), sizeof(Vertex), BufferType::Vertex, false, "Vertex Buffer");
    out.IndexBuffer = context->CreateBuffer(out.IndexCount * sizeof(uint32_t), 0, BufferType::Index, false, "Index Buffer");
    
    for (int i = 0; i < 3; i++) {
        out.ModelBuffer[i] = context->CreateBuffer(256, 0, BufferType::Constant, false, "Model Buffer");
        out.ModelBuffer[i]->BuildConstantBuffer();
    }

    // MATERIAL
    cgltf_material* material = primitive->material;
    Material meshMaterial = {};
    out.MaterialIndex = Materials.size();

    Bitmap albedoImage;
    Bitmap normalImage;
    Bitmap pbrImage;
    Bitmap emissiveImage;
    Bitmap aoImage;

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
                albedoImage.LoadFromFile(texturePath);
                meshMaterial.AlbedoTexture = context->CreateTexture(albedoImage.Width, albedoImage.Height, TextureFormat::RGBA8, TextureUsage::ShaderResource, true, meshMaterial.AlbedoPath);
                meshMaterial.AlbedoTexture->BuildShaderResource();
                uploader.CopyHostToDeviceTexture(albedoImage, meshMaterial.AlbedoTexture);
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
                normalImage.LoadFromFile(texturePath);
                meshMaterial.NormalTexture = context->CreateTexture(normalImage.Width, normalImage.Height, TextureFormat::RGBA8, TextureUsage::ShaderResource, true, meshMaterial.NormalPath);
                meshMaterial.NormalTexture->BuildShaderResource();
                uploader.CopyHostToDeviceTexture(normalImage, meshMaterial.NormalTexture);
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
                pbrImage.LoadFromFile(texturePath);
                meshMaterial.PBRTexture = context->CreateTexture(pbrImage.Width, pbrImage.Height, TextureFormat::RGBA8, TextureUsage::ShaderResource, true, meshMaterial.MetallicRoughnessPath);
                meshMaterial.PBRTexture->BuildShaderResource();
                uploader.CopyHostToDeviceTexture(pbrImage, meshMaterial.PBRTexture);
                TextureCache[texturePath] = meshMaterial.PBRTexture;
            }
        } else if (material->specular.specular_texture.texture) {
            std::string texturePath = Directory + '/' + std::string(material->specular.specular_texture.texture->image->uri);
            meshMaterial.MetallicRoughnessPath = texturePath;
            meshMaterial.HasMetallicRoughness = true;
            if (TextureCache.count(texturePath) != 0) {
                meshMaterial.PBRTexture = TextureCache[texturePath];
            } else{
                pbrImage.LoadFromFile(texturePath);
                meshMaterial.PBRTexture = context->CreateTexture(pbrImage.Width, pbrImage.Height, TextureFormat::RGBA8, TextureUsage::ShaderResource, true, meshMaterial.MetallicRoughnessPath);
                meshMaterial.PBRTexture->BuildShaderResource();
                uploader.CopyHostToDeviceTexture(pbrImage, meshMaterial.PBRTexture);
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
                emissiveImage.LoadFromFile(texturePath);
                meshMaterial.EmissiveTexture = context->CreateTexture(emissiveImage.Width, emissiveImage.Height, TextureFormat::RGBA8, TextureUsage::ShaderResource, true, meshMaterial.EmissivePath);
                meshMaterial.EmissiveTexture->BuildShaderResource();
                uploader.CopyHostToDeviceTexture(emissiveImage, meshMaterial.EmissiveTexture);
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
                aoImage.LoadFromFile(texturePath);
                meshMaterial.AOTexture = context->CreateTexture(aoImage.Width, aoImage.Height, TextureFormat::RGBA8, TextureUsage::ShaderResource, true, meshMaterial.AOPath);
                meshMaterial.AOTexture->BuildShaderResource();
                uploader.CopyHostToDeviceTexture(aoImage, meshMaterial.AOTexture);
                TextureCache[texturePath] = meshMaterial.AOTexture;
            }
        }
    }

    uploader.CopyHostToDeviceLocal(vertices.data(), vertices.size() * sizeof(Vertex), out.VertexBuffer);
    uploader.CopyHostToDeviceLocal(indices.data(), indices.size() * sizeof(uint32_t), out.IndexBuffer);
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
    Materials.push_back(meshMaterial);

    for (int i = 0; i < 3; i++) {
        void *pData;
        out.ModelBuffer[i]->Map(0, 0, &pData);
        memcpy(pData, &temp, sizeof(ModelData));
        out.ModelBuffer[i]->Unmap(0, 0);
    }

    if (meshMaterial.HasAlbedo) {
        context->GenerateMips(meshMaterial.AlbedoTexture);
    }
    if (meshMaterial.HasNormal) {
        context->GenerateMips(meshMaterial.NormalTexture);
    }
    if (meshMaterial.HasMetallicRoughness) {
        context->GenerateMips(meshMaterial.PBRTexture);
    }
    if (meshMaterial.HasAO) {
        context->GenerateMips(meshMaterial.AOTexture);
    }
    if (meshMaterial.HasEmissive) {
        context->GenerateMips(meshMaterial.EmissiveTexture);
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
