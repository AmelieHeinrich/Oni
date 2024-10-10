/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-30 14:04:37
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

enum class ShaderType : uint32_t
{
    None = 0,
    Vertex = 1,
    Fragment = 2,
    Compute = 3,
    Mesh = 4,
    Amplification = 5,
    Raytracing = 6,
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
