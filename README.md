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

### Techniques

- Forward shading
- IBL
- Normal, PBR, emissive, AO textures
- Skybox renderer

### Post Processing
- Color correction
- Tonemapping

### Misc
- Resource inspector
- Screenshot system

## WIP:

- Auto exposure

## Screenshots

### Physically Based Rendering - IBL

![](screenshots/DamgedHelmetPBR.png)
