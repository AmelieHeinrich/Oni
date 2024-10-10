//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-18 14:13:29
//

#include "hot_reloadable_pipeline.hpp"

#include "core/log.hpp"
#include "core/shader_loader.hpp"
#include "core/file_system.hpp"

HotReloadablePipeline::~HotReloadablePipeline()
{
    _shaders.clear();
}

void HotReloadablePipeline::ReflectRootSignature(bool reflect)
{
    _reflectRootSignature = reflect;
}

void HotReloadablePipeline::AddShaderWatch(const std::string& path, const std::string& entryPoint, ShaderType type)
{
    if (!FileSystem::Exists(path)) {
        Logger::Error("Shader doesn't exist!");
    }

    ShaderWatch watch;
    watch.Path = path;
    watch.EntryPoint = entryPoint;
    watch.Watch.Load(path);
    watch.Bytecode = ShaderLoader::GetFromCache(path);
    
    _shaders[type] = watch;
}

ShaderBytecode HotReloadablePipeline::GetBytecode(ShaderType type)
{
    return _shaders[type].Bytecode;
}

void HotReloadablePipeline::Build(RenderContext::Ptr context)
{
    Signature.reset();

    switch (_type) {
        case PipelineType::Compute: {
            if (_reflectRootSignature) {
                Signature = context->CreateRootSignature();
                Signature->ReflectFromComputeShader(GetBytecode(ShaderType::Compute));
            } else {
                Signature = context->CreateRootSignature(SignatureInfo);
            }
            ComputePipeline = context->CreateComputePipeline(GetBytecode(ShaderType::Compute), Signature);
            break;
        }
        case PipelineType::Graphics: {
            if (_reflectRootSignature) {
                Signature = context->CreateRootSignature();
                Signature->ReflectFromGraphicsShader(GetBytecode(ShaderType::Vertex), GetBytecode(ShaderType::Fragment));
            } else {
                Signature = context->CreateRootSignature(SignatureInfo);
            }
            Specs.Bytecodes[ShaderType::Vertex] = GetBytecode(ShaderType::Vertex);
            Specs.Bytecodes[ShaderType::Fragment] = GetBytecode(ShaderType::Fragment);
            Specs.Signature = Signature;
            GraphicsPipeline = context->CreateGraphicsPipeline(Specs);
            break;
        }
        case PipelineType::Mesh: {
            if (_reflectRootSignature) {
                Logger::Error("Shader reflection for mesh shaders is currently unsupported!");
            } else {
                Signature = context->CreateRootSignature(SignatureInfo);
            }
            if (Specs.UseAmplification) {
                Specs.Bytecodes[ShaderType::Amplification] = GetBytecode(ShaderType::Amplification);
            }
            Specs.Bytecodes[ShaderType::Mesh] = GetBytecode(ShaderType::Mesh);
            Specs.Bytecodes[ShaderType::Fragment] = GetBytecode(ShaderType::Fragment);
            Specs.Signature = Signature;
            MeshPipeline = context->CreateMeshPipeline(Specs);
            break;
        }
        case PipelineType::Raytracing: {
            if (_reflectRootSignature) {
                Logger::Error("Shader reflection for raytracing shaders is currently unsupported!");
            } else {
                Signature = context->CreateRootSignature(SignatureInfo);
            }
            RTSpecs.LibBytecode = GetBytecode(ShaderType::Raytracing);
            RTSpecs.Signature = Signature;
            RTPipeline = context->CreateRaytracingPipeline(RTSpecs);
            break;
        }
    }
}

void HotReloadablePipeline::CheckForRebuild(RenderContext::Ptr context, const std::string &name)
{
    for (auto& shader : _shaders) {
        ShaderWatch& watch = shader.second;

        if (watch.Watch.Check()) {
            Logger::Info("[HOT RELOAD PIPELINE] Hot reloading pipeline %s", name.c_str());
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
                case PipelineType::Mesh: {
                    ShaderBytecode temp;
                    if (!ShaderCompiler::CompileShader(watch.Path, watch.EntryPoint, shader.first, temp)) {
                        return;
                    } else {
                        watch.Bytecode.bytecode = temp.bytecode;
                    }

                    MeshPipeline.reset();
                    Build(context);
                    break;
                }
                case PipelineType::Raytracing: {
                    ShaderBytecode temp;
                    if (!ShaderCompiler::CompileShader(watch.Path, watch.EntryPoint, shader.first, temp)) {
                        return;
                    } else {
                        watch.Bytecode.bytecode = temp.bytecode;
                    }

                    RTPipeline.reset();
                    Build(context);
                    break;
                }
            }
        }
    }
}
