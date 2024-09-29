/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-31 13:28:02
 */

#pragma once

#include <string>
#include <array>
#include <glm/glm.hpp>
#include <cgltf/cgltf.h>

#include "rhi/render_context.hpp"

struct AABB
{
    glm::vec3 Center;
    float Extent;
};

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
    std::array<Buffer::Ptr, FRAMES_IN_FLIGHT> ModelBuffer;

    uint32_t VertexCount;
    uint32_t IndexCount;
    uint32_t MaterialIndex;

    glm::mat4 PrevTransform;
    glm::mat4 Transform;
    AABB BoundingBox;
    std::string Name;
};

class Model
{
public:
    std::vector<Primitive> Primitives;
    std::vector<Material> Materials;
    std::unordered_map<std::string, Texture::Ptr> TextureCache;

    uint32_t VertexCount;
    uint32_t IndexCount;

    std::string Directory;
    std::string Name;

    void Load(RenderContext::Ptr renderContext, const std::string& path);
    ~Model() = default;

    void ApplyTransform(glm::mat4 transform);
private:
    void ProcessPrimitive(RenderContext::Ptr context, cgltf_primitive *primitive, glm::mat4 transform, std::string name);
    void ProcessNode(RenderContext::Ptr context, cgltf_node *node, glm::mat4 transform);
};
