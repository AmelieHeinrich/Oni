/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-27 13:36:21
 */

#include "core/log.hpp"
#include "core/window.hpp"

#include "rhi/device.hpp"

int main(void)
{
    Logger::Init();

    Device device;

    Window window(1280, 720, "Oni | <D3D12> | <WINDOWS>");
    while (window.IsOpen()) {
        window.Update();
    }

    Logger::Exit();
}
