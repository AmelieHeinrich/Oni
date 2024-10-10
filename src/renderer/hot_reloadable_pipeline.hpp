//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-18 14:08:41
//

#pragma once

#include "core/file_watch.hpp"
#include "rhi/render_context.hpp"
#include "rhi/graphics_pipeline.hpp"
#include "rhi/compute_pipeline.hpp"
#include "rhi/mesh_pipeline.hpp"
#include "rhi/raytracing/raytracing_pipeline.hpp"

#include <unordered_map>

enum class PipelineType
{
    Graphics,
    Compute,
    Mesh,
    Raytracing
    // Graph Node
};

class HotReloadablePipeline
{
public:
    HotReloadablePipeline(PipelineType type)
        : _type(type)
    {
    }
    ~HotReloadablePipeline();

    void ReflectRootSignature(bool reflect);
    void AddShaderWatch(const std::string& path, const std::string& entryPoint, ShaderType type);
    ShaderBytecode GetBytecode(ShaderType type);

    void Build(RenderContext::Ptr context);
    void CheckForRebuild(RenderContext::Ptr context, const std::string &name = "");

    GraphicsPipelineSpecs Specs;
    RaytracingPipelineSpecs RTSpecs;
    RootSignatureBuildInfo SignatureInfo;

    GraphicsPipeline::Ptr GraphicsPipeline;
    ComputePipeline::Ptr ComputePipeline;
    MeshPipeline::Ptr MeshPipeline;
    RaytracingPipeline::Ptr RTPipeline;

    RootSignature::Ptr Signature;
private:
    PipelineType _type;
    bool _reflectRootSignature = true;

    struct ShaderWatch
    {
        FileWatch Watch;
        ShaderBytecode Bytecode;
        std::string Path;
        std::string EntryPoint;
    };

    std::unordered_map<ShaderType, ShaderWatch> _shaders;
};
