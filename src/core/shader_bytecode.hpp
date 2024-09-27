/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-30 14:04:37
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

enum class ShaderType
{
    Vertex,
    Fragment,
    Compute,
    // TODO(ahi): Mesh
    // TODO(ahi): Raytracing
    // TODO(ahi): Node
};

struct ShaderBytecode
{
    ShaderType type;
    std::vector<uint32_t> bytecode;
};

class ShaderCompiler
{
public:
    static bool CompileShader(const std::string& path, const std::string& entryPoint, ShaderType type, ShaderBytecode& bytecode);
};
