# Oni : D3D12 research renderer and playground

Oni is an experimental sandbox renderer written in C++ with D3D12. It's purpose is for it to be used as a portfolio piece once I go back to job finding.
Oni makes heavy use of compute shaders for its rendering. Anything that isn't a draw call will be put into a compute shader.

![](screenshots/Bistro.png)
![](screenshots/Sponza.png)

## Authors

- Amélie Heinrich (amelie.heinrich.dev@gmail.com)

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
- Copy the contents of the bin folder in build/windows/x64/{debug/release}/
- Copy the D3D12 folder in build/windows/x64/{debug/release}/
- xmake run
- That's it!

## Features:

### Techniques

- Point lights
- Directional lights
- Forward shading
- Deferred rendering:
    - Depth Buffer R32Typeless
    - Normals RGBA16Float
    - Albedo + Emissive RGBA8Unorm
    - PBR + AO RGBA8Unorm
- PBR lighting and materials
- Environment mapping
- IBL
- Shadow mapping (directional)

### GPU Driven Rendering
- Bindless resources via SM6.6

### Post Processing
- TAA for static/dynamic scenes
- Motion Blur
- Chromatic Aberration
- FFT Bloom
- Color grading
- Tonemapping

### Misc
- Resource inspector
- VRAM/RAM/Battery usage UI
- Screenshot system
- Mipmap generation through compute
- Simple scene editor to add/move lights around
- Debug renderer (lines)
- Log window
- Shader hot reloading & reflection
- Texture compression (can be generated, not loaded in engine yet)

## WIP

- Motion Blur

## Screenshots

### UI (as of 9/14/2024)

![](screenshots/engine/Screenshot%20Fri%20Sep%2027%2022_56_42%202024.png)

## Third party dependencies

- [D3D12MA](https://gpuopen.com/d3d12-memory-allocator/)
- [glm](https://github.com/g-truc/glm)
- [ImGui](https://github.com/ocornut/ImGui)
- [ImGuizmo](https://github.com/CedricGuillemet/ImGuizmo)
- [nvtt](https://github.com/castano/nvidia-texture-tools)
- [optick](https://github.com/bombomby/optick)
- [stb](https://github.com/nothings/stb)
- [Agility SDK](https://devblogs.microsoft.com/directx/directx12agility/)
- [Assimp](https://github.com/assimp/assimp)
