/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-31 13:28:02
 */

#pragma once

#include <string>
#include <array>
#include <queue>
#include <glm/glm.hpp>
#include <cgltf/cgltf.h>

#include "rhi/render_context.hpp"
#include "core/transform.hpp"

#define MAX_MESHLET_TRIANGLES 124
#define MAX_MESHLET_VERTICES 64

struct AABB
{
    glm::vec3 Min;
    glm::vec3 Max;

    glm::vec3 Center;
    glm::vec3 Extent;
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

struct MeshletBounds
{
    /* bounding sphere, useful for frustum and occlusion culling */
	glm::vec3 center;
	float radius;

	/* normal cone, useful for backface culling */
	glm::vec3 cone_apex;
	glm::vec3 cone_axis;
	float cone_cutoff; /* = cos(angle/2) */
};

struct Primitive
{
    Buffer::Ptr VertexBuffer;
    Buffer::Ptr IndexBuffer;
    Buffer::Ptr MeshletBuffer;
    Buffer::Ptr MeshletVertices;
    Buffer::Ptr MeshletTriangles;
    Buffer::Ptr MeshletBounds;

    BLAS::Ptr BottomLevelAS;

    uint32_t VertexCount;
    uint32_t IndexCount;
    uint32_t MeshletCount;

    std::array<Buffer::Ptr, FRAMES_IN_FLIGHT> ModelBuffer;

    Transform PrevTransform;
    Transform Transform;
    std::string Name;

    uint32_t MaterialIndex;

    AABB BoundingBox;
};

class Model
{
public:
    std::vector<Primitive> Primitives;
    std::vector<Material> Materials;
    std::unordered_map<std::string, Texture::Ptr> TextureCache;

    uint32_t VertexCount;
    uint32_t IndexCount;
    uint32_t MeshletCount;

    std::string Directory;
    std::string Name;

    void Load(RenderContext::Ptr renderContext, const std::string& path);
    ~Model() = default;

    void ApplyTransform(glm::mat4 transform);
private:
    void ProcessPrimitive(RenderContext::Ptr context, cgltf_primitive *primitive, Transform transform, std::string name);
    void ProcessNode(RenderContext::Ptr context, cgltf_node *node, Transform transform);
};
