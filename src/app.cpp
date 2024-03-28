/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-28 20:06:52
 */

#include "app.hpp"

App::App()
{
    Logger::Init();

    _window = std::make_unique<Window>(1280, 720, "Oni | <D3D12> | <WINDOWS>");
    _renderContext = std::make_unique<RenderContext>(_window->GetHandle());
}

App::~App()
{
    Logger::Exit();
}

void App::Run()
{
    while (_window->IsOpen()) {
        _window->Update();
    }
}
