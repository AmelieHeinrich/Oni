# Oni : D3D12 research renderer and playground

Oni is an experimental sandbox renderer written in C++ with D3D12. It's purpose is for it to be used as a portfolio piece once I go back to job finding.

## Authors

- Amélie Heinrich (amelie.heinrich.dev@gmail.com)

## Requirements

- [xmake](https://xmake.io/#/)
- Windows SDK Latest
- Visual Studio 2022 + ATL Toolkit
- GPU with the following features:
    - DXR
    - Mesh shaders
    - Work graphs

## Building

- xmake
- Copy the contents of the bin folder in build/windows/x64/{debug/release}/
- Copy the contents of the D3D12 folder in build/windows/x64/{debug/release}/
- xmake run
- That's it!

## Features:

- Forward shading
- Skybox

## WIP:

- IBL

## Roadmap:

### Techniques:
- Visibility buffer
- Cascaded shadow maps (CSM)
- Reflective shadow maps (RSM)
- Volumetrics (clouds, fog, god rays)
- Raytraced shadows
- Raytraced reflections
- Raytraced global illumination
- Raytraced ambient occlsuion
- Clustered forward/tiled light culling
- Shell texturing for foliage
- Ocean rendering
- Terrain generation

### GPU driven rendering
- Mesh shaders
- Work graphs
- Bindless
- Frustum/Occlusion culling
- (Multi) Draw indirect
- GPU instancing

### Post processing
- Ground truth ambient occlusion (GTAO)
- Screen space global illumination (SSGI)
- Temporal anti aliasing (TAA)
- Screen space reflections (SSR)
- Auto exposure
- Constrast adaptive sharpness (CAS)
- Bloom
- Kuwahara filter
- Depth of field (DOF)

### Other
- Profiler
- Render graph
- Multithreading
- Developer console
- Screenshot system
- Custom shader post processing system
- Shader graph
