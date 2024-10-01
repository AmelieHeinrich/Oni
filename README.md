# Oni : a DirectX 12 renderer written in C++

Oni is a modern graphics engine written in C++ with the DirectX 12 API. Its goal is to showcase the latest advancements in real-time rendering.
Oni makes heavy use of compute shaders to accelerate the rendering pipeline -- everything that isn't a draw call is done through compute.

## Requirements

- [xmake](https://xmake.io/#/)
- Windows SDK Latest
- Visual Studio 2022 + ATL Toolkit
- HLSL Shader Model 6.6
- GPU with, ideally, the following features:
    - DXR
    - Mesh shaders
    - Work graphs

## Building

- xmake
- xmake f --mode={debug/release}
- Copy the contents of the bin folder in build/windows/x64/{debug/release}/
- Copy the D3D12 folder in build/windows/x64/{debug/release}/
- xmake run

## Screenshots

![](screenshots/engine/Screenshot%20Fri%20Sep%2027%2022_56_42%202024.png) ![](screenshots/NewBistro.png) ![](screenshots/Multiple%20Models.png)

## Renderer features

- Deferred rendering
- Shadow mapping
- Bloom (experimental, doesn't work well or look good)
- HDR rendering
- Color grading
- TAA (Temporal Anti-Aliasing)
- Film Grain
- Debug renderer (lines, motion vector visualizer)
- PBR and emissive material workflow
- Point, directional lights

## Other features
- UI powered by ImGui
- In-engine screenshot system
- Shader hot reloading and caching
- In-engine BC7 texture compressor using custom texture format

## WIP

- Cascaded shadow mapping

## Dependencies

- [D3D12MA](https://gpuopen.com/d3d12-memory-allocator/)
- [glm](https://github.com/g-truc/glm)
- [ImGui](https://github.com/ocornut/ImGui)
- [ImGuizmo](https://github.com/CedricGuillemet/ImGuizmo)
- [nvtt](https://github.com/castano/nvidia-texture-tools)
- [optick](https://github.com/bombomby/optick)
- [stb](https://github.com/nothings/stb)
- [Agility SDK](https://devblogs.microsoft.com/directx/directx12agility/)
- [cgltf](https://github.com/jkuhlmann/cgltf)
