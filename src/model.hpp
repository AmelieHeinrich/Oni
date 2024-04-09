/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-31 13:28:02
 */

#pragma once

#include <string>
#include <glm/glm.hpp>

#include "rhi/render_context.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/pbrmaterial.h>

struct Vertex
{
    glm::vec3 Position;
    glm::vec2 UV;
    glm::vec3 Normals;
};

struct Material
{
    bool HasAlbedo = false;
    bool HasNormal = false;
    bool HasMetallicRoughness = false;
    bool HasEmissive = false;
    bool HasAO = false;

    std::string AlbedoPath;
    std::string NormalPath;
    std::string MetallicRoughnessPath;
    std::string EmissivePath;
    std::string AOPath;

    Texture::Ptr AlbedoTexture;
    Texture::Ptr NormalTexture;
    Texture::Ptr PBRTexture;
    Texture::Ptr EmissiveTexture;
    Texture::Ptr AOTexture;

    glm::vec3 FlatColor;
};

struct Primitive
{
    Buffer::Ptr VertexBuffer;
    Buffer::Ptr IndexBuffer;

    uint32_t VertexCount;
    uint32_t IndexCount;
    uint32_t MaterialIndex;

    glm::mat4 Transform;
};

class Model
{
public:
    std::vector<Primitive> Primitives;
    std::vector<Material> Materials;

    uint32_t VertexCount;
    uint32_t IndexCount;

    std::string Directory;

    void Load(RenderContext::Ptr renderContext, const std::string& path);
    ~Model() = default;

private:
    void ProcessPrimitive(RenderContext::Ptr renderContext, aiMesh *mesh, const aiScene *scene, glm::mat4 transform);
    void ProcessNode(RenderContext::Ptr renderContext, aiNode *node, const aiScene *scene);
};
