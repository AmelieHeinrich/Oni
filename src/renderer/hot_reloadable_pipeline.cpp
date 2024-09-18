//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-18 14:13:29
//

#include "hot_reloadable_pipeline.hpp"

#include "core/log.hpp"

HotReloadablePipeline::~HotReloadablePipeline()
{
    _shaders.clear();
}

void HotReloadablePipeline::AddShaderWatch(const std::string& path, const std::string& entryPoint, ShaderType type)
{
    ShaderWatch watch;
    watch.Path = path;
    watch.EntryPoint = entryPoint;
    watch.Watch.Load(path);
    ShaderCompiler::CompileShader(path, entryPoint, type, watch.Bytecode);
    
    _shaders[type] = watch;
}

ShaderBytecode HotReloadablePipeline::GetBytecode(ShaderType type)
{
    return _shaders[type].Bytecode;
}

void HotReloadablePipeline::Build(RenderContext::Ptr context)
{
    switch (_type) {
        case PipelineType::Compute: {
            ComputePipeline = context->CreateComputePipeline(GetBytecode(ShaderType::Compute));
            break;
        }
        case PipelineType::Graphics: {
            Specs.Bytecodes[ShaderType::Vertex] = GetBytecode(ShaderType::Vertex);
            Specs.Bytecodes[ShaderType::Fragment] = GetBytecode(ShaderType::Fragment);
            GraphicsPipeline = context->CreateGraphicsPipeline(Specs);
            break;
        }
    }
}

void HotReloadablePipeline::CheckForRebuild(RenderContext::Ptr context, const std::string &name)
{
    for (auto& shader : _shaders) {
        ShaderWatch& watch = shader.second;

        if (watch.Watch.Check()) {
            Logger::Info("Hot reloading pipeline %s", name.c_str());
            switch (_type) {
                case PipelineType::Graphics: {
                    ShaderBytecode temp;
                    if (!ShaderCompiler::CompileShader(watch.Path, watch.EntryPoint, shader.first, temp)) {
                        return;
                    } else {
                        watch.Bytecode.bytecode = temp.bytecode;
                    }

                    GraphicsPipeline.reset();
                    Build(context);
                    break;
                }
                case PipelineType::Compute: {
                    ShaderBytecode temp;
                    if (!ShaderCompiler::CompileShader(watch.Path, watch.EntryPoint, shader.first, temp)) {
                        return;
                    } else {
                        watch.Bytecode.bytecode = temp.bytecode;
                    }

                    ComputePipeline.reset();
                    Build(context);
                    break;
                }
            }
        }
    }
}
