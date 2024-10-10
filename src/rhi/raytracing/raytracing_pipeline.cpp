//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-10 21:20:22
//

#include "raytracing_pipeline.hpp"

#include "core/log.hpp"
#include "d3dx12/d3dx12.h"

RaytracingPipeline::RaytracingPipeline(Device::Ptr device, Allocator::Ptr allocator, DescriptorHeap::Heaps& heaps, RaytracingPipelineSpecs& specs)
{
    if (specs.Signature == nullptr) {
        Logger::Error("Raytracing Pipeline MUST have a provided root signature at creation.");
        return;
    }

    D3D12_DXIL_LIBRARY_DESC lib = {};
    lib.DXILLibrary.BytecodeLength = specs.LibBytecode.bytecode.size() * sizeof(uint32_t);
    lib.DXILLibrary.pShaderBytecode = specs.LibBytecode.bytecode.data();
    lib.NumExports = 0;

    D3D12_HIT_GROUP_DESC hitGroup = {};
    hitGroup.HitGroupExport = L"HitGroup";
    hitGroup.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
    hitGroup.ClosestHitShaderImport = L"ClosestHit";

    D3D12_RAYTRACING_SHADER_CONFIG shaderCfg = {};
    shaderCfg.MaxAttributeSizeInBytes = specs.AttributeSize;
    shaderCfg.MaxPayloadSizeInBytes = specs.PayloadSize;

    D3D12_GLOBAL_ROOT_SIGNATURE globalSig = {};
    globalSig.pGlobalRootSignature = specs.Signature->GetSignature();

    D3D12_RAYTRACING_PIPELINE_CONFIG pipelineCfg = {};
    pipelineCfg.MaxTraceRecursionDepth = specs.MaxTraceRecursionDepth;

    D3D12_STATE_SUBOBJECT subObjects[] = {
        { D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY, &lib },
        { D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP, &hitGroup },
        { D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG, &shaderCfg },
        { D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE, &globalSig },
        { D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG, &pipelineCfg }
    };

    D3D12_STATE_OBJECT_DESC desc = {};
    desc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
    desc.NumSubobjects = 5;
    desc.pSubobjects = subObjects;

    HRESULT result = device->GetDevice()->CreateStateObject(&desc, IID_PPV_ARGS(&_pipeline));
    if (FAILED(result)) {    
        Logger::Error("Failed to create raytracing pipeline!");
    }

   _idBuffer = std::make_shared<Buffer>(device, allocator, heaps, 3 * D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT, BufferType::Constant, false, "ID Buffer");

   ID3D12StateObjectProperties* props;
   _pipeline->QueryInterface(&props);

   void *pData;
   auto writeId = [&](const wchar_t* name) {
       void* id = props->GetShaderIdentifier(name);
       memcpy(pData, id, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
       pData = static_cast<char*>(pData) + D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
   };

   _idBuffer->Map(0, 0, &pData);
   writeId(L"RayGeneration");
   writeId(L"Miss");
   writeId(L"HitGroup");
   _idBuffer->Unmap(0, 0);

   props->Release();
}

RaytracingPipeline::~RaytracingPipeline()
{
    _pipeline->Release();
}
