/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-31 13:34:44
 */

#include "model.hpp"

#include "core/image.hpp"
#include "core/log.hpp"

void Model::ProcessPrimitive(RenderContext::Ptr renderContext, aiMesh *mesh, const aiScene *scene, glm::mat4 transform)
{
    Primitive out;
    out.Transform = transform;

    // Geometry
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    for (int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;

        vertex.Position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        if (mesh->HasNormals()) {
            vertex.Normals = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
        }
        if (mesh->mTextureCoords[0]) {
            vertex.UV = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
        }
        
        vertices.push_back(vertex);
    }

    for (int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    out.VertexCount = vertices.size();
    out.IndexCount = indices.size();

    out.VertexBuffer = renderContext->CreateBuffer(out.VertexCount * sizeof(Vertex), sizeof(Vertex), BufferType::Vertex, false, "Vertex Buffer");
    out.IndexBuffer = renderContext->CreateBuffer(out.IndexCount * sizeof(uint32_t), 0, BufferType::Index, false, "Index Buffer");

    // MATERIAL
    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
    Material meshMaterial = {};
    out.MaterialIndex = Materials.size();

    Image albedoImage;
    Image normalImage;
    Image pbrImage;
    Image emissiveImage;
    Image aoImage;

    aiColor3D flatColor(1.0f, 1.0f, 1.0f);
    material->Get(AI_MATKEY_COLOR_DIFFUSE, flatColor);
    meshMaterial.FlatColor = glm::vec3(flatColor.r, flatColor.g, flatColor.b);

    {
        aiString str;
        material->GetTexture(aiTextureType_DIFFUSE, 0, &str);
        if (str.length) {
            std::string texturePath = Directory + '/' + str.C_Str();
            meshMaterial.AlbedoPath = texturePath;
            meshMaterial.HasAlbedo = true;
            albedoImage.LoadFromFile(texturePath);
        }
    }
    {
        aiString str;
        material->GetTexture(aiTextureType_NORMALS, 0, &str);
        if (str.length) {
            std::string texturePath = Directory + '/' + str.C_Str();
            meshMaterial.NormalPath = texturePath;
            meshMaterial.HasNormal = true;
            normalImage.LoadFromFile(texturePath);
        }
    }
    {
        aiString str;
        material->GetTexture(aiTextureType_UNKNOWN, 0, &str);
        if (str.length) {
            std::string texturePath = Directory + '/' + str.C_Str();
            meshMaterial.MetallicRoughnessPath = texturePath;
            meshMaterial.HasMetallicRoughness = true;
            pbrImage.LoadFromFile(texturePath);
        }
    }
    {
        aiString str;
        material->GetTexture(aiTextureType_EMISSIVE, 0, &str);
        if (str.length) {
            std::string texturePath = Directory + '/' + str.C_Str();
            meshMaterial.EmissivePath = texturePath;
            meshMaterial.HasEmissive = true;
            emissiveImage.LoadFromFile(texturePath);
        }
    }
    {
        aiString str;
        material->GetTexture(aiTextureType_LIGHTMAP, 0, &str);
        if (str.length) {
            std::string texturePath = Directory + '/' + str.C_Str();
            meshMaterial.AOPath = texturePath;
            meshMaterial.HasAO = true;
            aoImage.LoadFromFile(texturePath);
        }
    }

    Uploader uploader = renderContext->CreateUploader();
    if (meshMaterial.HasAlbedo) {
        meshMaterial.AlbedoTexture = renderContext->CreateTexture(albedoImage.Width, albedoImage.Height, TextureFormat::RGBA8, TextureUsage::ShaderResource, true, meshMaterial.AlbedoPath);
        meshMaterial.AlbedoTexture->BuildShaderResource();
        uploader.CopyHostToDeviceTexture(albedoImage, meshMaterial.AlbedoTexture);
    }
    if (meshMaterial.HasNormal) {
        meshMaterial.NormalTexture = renderContext->CreateTexture(normalImage.Width, normalImage.Height, TextureFormat::RGBA8, TextureUsage::ShaderResource, true, meshMaterial.NormalPath);
        meshMaterial.NormalTexture->BuildShaderResource();
        uploader.CopyHostToDeviceTexture(normalImage, meshMaterial.NormalTexture);
    }
    if (meshMaterial.HasMetallicRoughness) {
        meshMaterial.PBRTexture = renderContext->CreateTexture(pbrImage.Width, pbrImage.Height, TextureFormat::RGBA8, TextureUsage::ShaderResource, true, meshMaterial.MetallicRoughnessPath);
        meshMaterial.PBRTexture->BuildShaderResource();
        uploader.CopyHostToDeviceTexture(pbrImage, meshMaterial.PBRTexture);
    }
    if (meshMaterial.HasEmissive) {
        meshMaterial.EmissiveTexture = renderContext->CreateTexture(emissiveImage.Width, emissiveImage.Height, TextureFormat::RGBA8, TextureUsage::ShaderResource, true, meshMaterial.EmissivePath);
        meshMaterial.EmissiveTexture->BuildShaderResource();
        uploader.CopyHostToDeviceTexture(emissiveImage, meshMaterial.EmissiveTexture);
    }
    if (meshMaterial.HasAO) {
        meshMaterial.AOTexture = renderContext->CreateTexture(aoImage.Width, aoImage.Height, TextureFormat::RGBA8, TextureUsage::ShaderResource, true, meshMaterial.AOPath);
        meshMaterial.AOTexture->BuildShaderResource();
        uploader.CopyHostToDeviceTexture(aoImage, meshMaterial.AOTexture);
    }
    uploader.CopyHostToDeviceLocal(vertices.data(), vertices.size() * sizeof(Vertex), out.VertexBuffer);
    uploader.CopyHostToDeviceLocal(indices.data(), indices.size() * sizeof(uint32_t), out.IndexBuffer);
    renderContext->FlushUploader(uploader);

    VertexCount += out.VertexCount;
    IndexCount += out.IndexCount;
    Materials.push_back(meshMaterial);

    if (meshMaterial.HasAlbedo) {
        renderContext->GenerateMips(meshMaterial.AlbedoTexture);
    }
    if (meshMaterial.HasNormal) {
        renderContext->GenerateMips(meshMaterial.NormalTexture);
    }
    if (meshMaterial.HasMetallicRoughness) {
        renderContext->GenerateMips(meshMaterial.PBRTexture);
    }
    if (meshMaterial.HasAO) {
        renderContext->GenerateMips(meshMaterial.AOTexture);
    }
    if (meshMaterial.HasEmissive) {
        renderContext->GenerateMips(meshMaterial.EmissiveTexture);
    }

    glm::vec3 Min = glm::vec3(mesh->mAABB.mMin.x, mesh->mAABB.mMin.y, mesh->mAABB.mMin.z);
    glm::vec3 Max = glm::vec3(mesh->mAABB.mMax.x, mesh->mAABB.mMax.y, mesh->mAABB.mMax.z);

    out.BoundingBox.Extent = ((Max - Min) * 0.5f).x;
    out.BoundingBox.Center = Min + out.BoundingBox.Extent;

    Primitives.push_back(out);
}

void Model::ProcessNode(RenderContext::Ptr renderContext, aiNode *node, const aiScene *scene)
{
    for (int i = 0; i < node->mNumMeshes; i++) {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        glm::mat4 transform(1.0f);
        ProcessPrimitive(renderContext, mesh, scene, transform);
    }

    for (int i = 0; i < node->mNumChildren; i++) {
        ProcessNode(renderContext, node->mChildren[i], scene);
    }
}

void Model::Load(RenderContext::Ptr renderContext, const std::string& path)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_FlipWindingOrder | aiProcess_CalcTangentSpace | aiProcess_PreTransformVertices | aiProcess_GenBoundingBoxes | aiProcess_GenSmoothNormals | aiProcess_OptimizeMeshes);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        Logger::Error("Failed to load model at path %s", path.c_str());
    }
    Directory = path.substr(0, path.find_last_of('/'));
    ProcessNode(renderContext, scene->mRootNode, scene);
    Logger::Info("Successfully loaded model at path %s", path.c_str());
}
