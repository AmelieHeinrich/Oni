//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-15 16:01:11
//

#include "scene.hpp"

void Scene::Update(RenderContext::Ptr context)
{
    if (context->GetDevice()->GetFeatures().Raytracing) {
        Instances.clear();
        for (auto& m : Models) {
            for (auto& primitive : m.Primitives) {
                Instances.push_back(primitive.RTInstance);
            }       
        }

        memcpy(pData, Instances.data(), Instances.size() * sizeof(RaytracingInstance));
    }
}

void Scene::Bake(RenderContext::Ptr context)
{
    if (context->GetDevice()->GetFeatures().Raytracing) {
        for (auto& m : Models) {
            for (auto& primitive : m.Primitives) {
                Instances.push_back(primitive.RTInstance);
            }       
        }

        InstanceBuffers = context->CreateBuffer(Instances.size() * sizeof(RaytracingInstance), sizeof(RaytracingInstance), BufferType::Constant, false, "Scene Instance Buffers");
        InstanceBuffers->BuildShaderResource();

        InstanceBuffers->Map(0, 0, &pData);
        memcpy(pData, Instances.data(), Instances.size() * sizeof(RaytracingInstance));

        // Create TLAS
        TLAS = context->CreateTLAS(InstanceBuffers, Instances.size(), "Scene TLAS");

        Uploader uploader = context->CreateUploader();
        uploader.BuildTLAS(TLAS);
        context->FlushUploader(uploader);

        TLAS->FreeScratch();
    }
}
