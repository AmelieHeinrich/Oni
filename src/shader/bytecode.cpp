/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-30 14:09:24
 */

#include "bytecode.hpp"

#include <core/file_system.hpp>
#include <core/log.hpp>

#include <string>
#include <atlbase.h>
#include <dxcapi.h>
#include <wrl/client.h>

const char *GetProfileFromType(ShaderType type)
{
    switch (type) {
        case ShaderType::Vertex: {
            return "vs_6_6";
        }
        case ShaderType::Fragment: {
            return "ps_6_6";
        }
        case ShaderType::Compute: {
            return "cs_6_6";
        }
    }
    return "???";
}

void ShaderCompiler::CompileShader(const std::string& path, const std::string& entryPoint, ShaderType type, ShaderBytecode& bytecode)
{
    using namespace Microsoft::WRL;

    std::string source = FileSystem::ReadFile(path);

    wchar_t wideTarget[512];
    swprintf_s(wideTarget, 512, L"%hs", GetProfileFromType(type));
    
    wchar_t wideEntry[512];
    swprintf_s(wideEntry, 512, L"%hs", entryPoint.c_str());

    ComPtr<IDxcUtils> pUtils;
    ComPtr<IDxcCompiler> pCompiler;
    if (!SUCCEEDED(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&pUtils)))) {
        Logger::Error("DXC: Failed to create DXC utils instance!");
    }
    if (!SUCCEEDED(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&pCompiler)))) {
        Logger::Error("DXC: Failed to create DXC compiler instance!");
    }

    ComPtr<IDxcIncludeHandler> pIncludeHandler;
    if (!SUCCEEDED(pUtils->CreateDefaultIncludeHandler(&pIncludeHandler))) {
        Logger::Error("DXC: Failed to create default include handler!");
    }

    ComPtr<IDxcBlobEncoding> pSourceBlob;
    if (!SUCCEEDED(pUtils->CreateBlob(source.c_str(), source.size(), 0, &pSourceBlob))) {
        Logger::Error("DXC: Failed to create output blob!");
    }

    LPCWSTR pArgs[] = {
        L"-Zs",
        L"-Fd",
        L"-Fre"
    };

    ComPtr<IDxcOperationResult> pResult;
    if (!SUCCEEDED(pCompiler->Compile(pSourceBlob.Get(), L"Shader", wideEntry, wideTarget, pArgs, ARRAYSIZE(pArgs), nullptr, 0, pIncludeHandler.Get(), &pResult))) {
        Logger::Error("DXC: Failed to compile shader!");
    }

    ComPtr<IDxcBlobEncoding> pErrors;
    pResult->GetErrorBuffer(&pErrors);

    if (pErrors && pErrors->GetBufferSize() != 0)
    {
        ComPtr<IDxcBlobUtf8> pErrorsU8;
        pErrors->QueryInterface(IID_PPV_ARGS(&pErrorsU8));
        Logger::Error("Shader errors:%s", (char*)pErrorsU8->GetStringPointer());
    }

    HRESULT Status;
    pResult->GetStatus(&Status);

    ComPtr<IDxcBlob> pShaderBlob;
    pResult->GetResult(&pShaderBlob);

    bytecode.type = type;
    bytecode.bytecode.resize(pShaderBlob->GetBufferSize() / sizeof(uint32_t));
    memcpy(bytecode.bytecode.data(), pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize());

    Logger::Info("DXC: Compiled shader %s", path.c_str());
}
