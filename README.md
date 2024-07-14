# Oni : D3D12 research renderer and playground

Oni is an experimental sandbox renderer written in C++ with D3D12. It's purpose is for it to be used as a portfolio piece once I go back to job finding.

![](screenshots/engine/Screenshot%20Thu%20Apr%2025%2000_06_00%202024.png)

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
- Copy the D3D12 folder in build/windows/x64/{debug/release}/
- xmake run
- That's it!

## Features:

### Techniques

- Forward shading
- PBR lighting model
- Blinn Phong lighting model
- IBL
- Normal, PBR, emissive, AO textures
- Skybox renderer
- Point, directional lights

### Post Processing
- Color correction (can be tuned)
- Tonemapping

### Misc
- Resource inspector
- Screenshot system
- Mipmap generation through compute

## WIP:

- Shadow mapping
- CPU side frustum culling
- Auto exposure

## Screenshots

### Physically Based Rendering - IBL

| Damaged Helmet | Bistro | Sponza |
|---|---|---|
| ![](screenshots/DamagedHelmetPBR.png) | ![](screenshots/engine/Screenshot%20Thu%20Apr%2025%2000_06_00%202024.png) | ![](screenshots/SponzaPBR.png) |

### Color correction

| Off | On |
|---|---|
| ![](screenshots/No%20CC.png) | ![](screenshots/With%20CC.png) |

### UI (as of 4/25/2024)

![](screenshots/RecentUI.png)
