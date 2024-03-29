# Oni : D3D12 research renderer and playground

Oni is an experimental sandbox renderer written in C++ with D3D12. It's purpose is for it to be used as a portfolio piece once I go back to job finding.

## Authors

- Amélie Heinrich (amelie.heinrich.dev@gmail.com)

## Requirements

- [xmake](https://xmake.io/#/)
- Windows SDK Latest
- GPU with the following features:
    - DXR
    - Mesh shaders
    - Work graphs

## Building

- xmake
- xmake run
- That's it!

## Roadmap:

### Techniques:
- Deferred rendering
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
