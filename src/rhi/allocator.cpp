/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-28 22:45:16
 */

#include "allocator.hpp"
#include "texture.hpp"

#include <core/log.hpp>
#include <ImGui/imgui.h>
#include <sstream>

#undef max

void GPUResource::ClearFromAllocationList()
{
    if (ParentAllocator) {
        ParentAllocator->_allocations.erase(std::find(ParentAllocator->_allocations.begin(), ParentAllocator->_allocations.end(), this));
    }
}

Allocator::Allocator(Device::Ptr devicePtr)
{
    D3D12MA::ALLOCATOR_DESC desc = {};
    desc.pAdapter = devicePtr->GetAdapter();
    desc.pDevice = devicePtr->GetDevice();

    HRESULT result = D3D12MA::CreateAllocator(&desc, &_allocator);
    if (FAILED(result)) {
        Logger::Error("D3D12: Failed to create memory allocator!");
    }

    Logger::Info("D3D12: Successfully created memory allocator");
}

Allocator::~Allocator()
{
    for (auto& allocation : _allocations) {
        delete allocation;
    }
    _allocator->Release();
}

GPUResource *Allocator::Allocate(D3D12MA::ALLOCATION_DESC *allocDesc, D3D12_RESOURCE_DESC *resourceDesc, D3D12_RESOURCE_STATES state, const std::string& name)
{
    GPUResource* resource = new GPUResource;
    resource->Name = name;
    resource->Size = resourceDesc->Width * std::max(resourceDesc->Height, UINT32(1)) * std::max(resourceDesc->DepthOrArraySize, UINT16(1));
    resource->ParentAllocator = this;
    if (resourceDesc->Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
        resource->Type = GPUResourceType::Buffer;
    } else if (resourceDesc->Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D && resourceDesc->DepthOrArraySize == 6) {
        resource->Type = GPUResourceType::CubeMap;
    } else if (resourceDesc->Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D) {
        resource->Type = GPUResourceType::Texture;
    }

    HRESULT result = _allocator->CreateResource(allocDesc, resourceDesc, state, nullptr, &resource->Allocation, IID_PPV_ARGS(&resource->Resource));
    if (FAILED(result)) {
        Logger::Error("D3D12: Failed to allocate resource!");
    }
    _allocations.push_back(resource);

    std::wstring resourceName = std::wstring(name.begin(), name.end());
    resource->Resource->SetName(resourceName.c_str());

    return resource;
}

void Allocator::OnGUI()
{
    ImGui::Begin("Resource Inspector");

    // Left
    ImGui::BeginChild("left pane", ImVec2(150, 0), ImGuiChildFlags_ResizeX | ImGuiChildFlags_Border);
    if (ImGui::TreeNodeEx("Textures", ImGuiTreeNodeFlags_Framed)) {
        for (auto& resource : _allocations) {
            if (resource->Type == GPUResourceType::Texture) {
                if (ImGui::Selectable(resource->Name.c_str())) {
                    _uiSelected = resource;
                }
            }
        }
        ImGui::TreePop();
    }
    if (ImGui::TreeNodeEx("Buffers", ImGuiTreeNodeFlags_Framed)) {
        for (auto& resource : _allocations) {
            if (resource->Type == GPUResourceType::Buffer) {
                if (ImGui::Selectable(resource->Name.c_str())) {
                    _uiSelected = resource;
                }
            }
        }
        ImGui::TreePop();
    }
    if (ImGui::TreeNodeEx("Cube Maps", ImGuiTreeNodeFlags_Framed)) {
        for (auto& resource : _allocations) {
            if (resource->Type == GPUResourceType::CubeMap) {
                if (ImGui::Selectable(resource->Name.c_str())) {
                    _uiSelected = resource;
                }
            }
        }
        ImGui::TreePop();
    }
    ImGui::EndChild();
    ImGui::SameLine();

    // Right
    ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()));
    if (_uiSelected != nullptr) {
        ImGui::Text("Resource: %s", _uiSelected->Name.c_str());
        ImGui::Separator();

        // Textures
        if (_uiSelected->Type == GPUResourceType::Texture) {
            ImGui::Text("Texture Size: (%d, %d)", _uiSelected->AttachedTexture->GetWidth(), _uiSelected->AttachedTexture->GetHeight());
            for (int i = 0; i < _uiSelected->AttachedTexture->GetMips(); i++) {
                std::stringstream ss;
                ss << "Mip " << i;
                if (ImGui::TreeNodeEx(ss.str().c_str(), ImGuiTreeNodeFlags_Framed)) {
                    if (_uiSelected->AttachedTexture->_srvs.size() > 0) {
                        if (_uiSelected->AttachedTexture->_srvs[i].Valid && _uiSelected->AttachedTexture->_dsv.Valid != true) {
                            if (_uiSelected->AttachedTexture->GetState(i) == D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE)
                                ImGui::Image((ImTextureID)_uiSelected->AttachedTexture->_srvs[i].GPU.ptr, ImVec2(256, 256));
                        }
                    } else {
                        ImGui::TextColored(ImVec4(1, 0, 0, 1), "> Mip preview unavailable");
                    }
                    ImGui::TreePop();
                }
            }
        }
    }
    ImGui::EndChild();

    ImGui::End();
}

Allocator::Stats Allocator::GetStats()
{
    D3D12MA::Budget budget;
    _allocator->GetBudget(&budget, nullptr);
    
    Allocator::Stats stats;
    stats.Used = budget.UsageBytes;
    stats.Total = budget.BudgetBytes;

    return stats;
}
