/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-28 20:06:52
 */

#include "app.hpp"

#include "shader/bytecode.hpp"
#include "core/image.hpp"

#include "model.hpp"

#include <ImGui/imgui.h>
#include <glm/gtc/type_ptr.hpp>

#include <ctime>
#include <cstdlib>

float random_float(float min, float max)
{
    float scale = rand() / (float)RAND_MAX; 
    return min + scale * ( max - min ); 
}

App::App()
    : _camera(1280, 720), _lastFrame(0.0f)
{
    Logger::Init();
    srand(time(NULL));

    _window = std::make_shared<Window>(1280 + 16, 720 + 39, "Oni | <D3D12> | <WINDOWS>");
    _window->OnResize([&](uint32_t width, uint32_t height) {
        _renderContext->Resize(width, height);
        _renderer->Resize(width, height);
        _camera.Resize(width, height);
    });

    _renderContext = std::make_shared<RenderContext>(_window);
    _renderer = std::make_unique<Renderer>(_renderContext);

    scene = {};

    Model sponza;
    sponza.Load(_renderContext, "assets/models/DamagedHelmet.gltf");

    scene.Models.push_back(sponza);

    for (int i = 0; i < 8; i++) {
        PointLight light;
        light.Position = glm::vec4(random_float(-4.0f, 4.0f), random_float(1.0f, 5.0f), random_float(-4.0f, 4.0f), 1.0f);
        light.Color = glm::vec4(random_float(0.1f, 1.0f), random_float(0.1f, 1.0f), random_float(0.0f, 1.0f), 1.0f);
        light.Brightness = 5.0f;
        scene.LightBuffer.PointLights[i] = light;
    }
    scene.LightBuffer.PointLightCount = 8;

    scene.LightBuffer.Sun.Position = glm::vec4(0.0f, 10.0f, 0.0f, 0.0f);
    scene.LightBuffer.Sun.Direction = glm::vec4(0.0f, -1.0f, 0.0f, 0.0f);
    scene.LightBuffer.Sun.Color = glm::vec4(0.1f);
    scene.LightBuffer.HasSun = 1;
}

App::~App()
{
    _renderContext->WaitForPreviousDeviceSubmit(CommandQueueType::Graphics);
    _renderContext->WaitForPreviousHostSubmit(CommandQueueType::Graphics);

    Logger::Exit();
}

void App::Run()
{
    while (_window->IsOpen()) {
        static float framesPerSecond = 0.0f;
        static float lastTime = 0.0f;
        float currentTime = GetTickCount() * 0.001f;
        ++framesPerSecond;
        if (currentTime - lastTime > 1.0f) {
            lastTime = currentTime;
            _fps = (int)framesPerSecond;
            framesPerSecond = 0;
        }

        float time = _dtTimer.GetElapsed();
        float dt = (time - _lastFrame) / 1000.0f;
        _lastFrame = time;

        _window->Update();

        if (ImGui::IsKeyPressed(ImGuiKey_F1)) {
            _showUI = !_showUI;
        }

        uint32_t width, height;
        _window->GetSize(width, height);

        _camera.Update(dt);
        _renderContext->BeginFrame();

        scene.View = _camera.View();
        scene.Projection = _camera.Projection();
        scene.CameraPosition = glm::vec4(_camera.GetPosition(), 1.0f);

        CommandBuffer::Ptr commandBuffer = _renderContext->GetCurrentCommandBuffer();
        Texture::Ptr texture = _renderContext->GetBackBuffer();

        // RENDER
        {
            _renderer->Render(scene, width, height, dt);
        }  

        // UI
        {
            commandBuffer->Begin();
            commandBuffer->ImageBarrier(texture, TextureLayout::RenderTarget);
            commandBuffer->BindRenderTargets({ texture }, nullptr);
            commandBuffer->BeginImGui(width, height);
            RenderOverlay();
            if (!_showUI) {
                RenderHelper();
            }
            commandBuffer->EndImGui();
            commandBuffer->ImageBarrier(texture, TextureLayout::Present);
            commandBuffer->End();
            _renderContext->ExecuteCommandBuffers({ commandBuffer }, CommandQueueType::Graphics);
        }

        // FLUSH
        {
            _renderContext->EndFrame();
            _renderContext->Present(true);
        }

        if (!_showUI) {
            _camera.Input(dt);
        }

        if ((_updateTimer.GetElapsed() / 1000.0f) > 1.0f) {
            _frameTime = clock() - time;
            _updateTimer.Restart();
        } 
    }
}

void App::RenderOverlay()
{
    if (_showUI) {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Debug")) {
                if (ImGui::MenuItem("Resource Inspector")) {
                    _showResourceInspector = !_showResourceInspector;
                }
                if (ImGui::MenuItem("Renderer Settings")) {
                    _showRendererSettings = !_showRendererSettings;
                }
                if (ImGui::MenuItem("Light Editor")) {
                    _showLightEditor = !_showLightEditor;
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        if (_showResourceInspector) {
            _renderContext->OnGUI();
        }
        if (_showRendererSettings) {
            _renderer->OnUI();
        }
        if (_showLightEditor) {
            ShowLightEditor();
        }

        _renderContext->OnOverlay();
    }
}

void App::RenderHelper()
{
    static bool p_open = true;

    ImGuiIO& io = ImGui::GetIO();
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
    const float PAD = 10.0f;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
    ImVec2 work_size = viewport->WorkSize;
    ImVec2 window_pos, window_pos_pivot;
    window_pos.x = (work_pos.x + PAD);
    window_pos.y = (work_pos.y + PAD);
    window_pos_pivot.x = 0.0f;
    window_pos_pivot.y = 0.0f;
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
    window_flags |= ImGuiWindowFlags_NoMove;

    ImGui::SetNextWindowBgAlpha(0.35f);
    ImGui::Begin("Example: Simple overlay", &p_open, window_flags);
    ImGui::Text("WASD + Mouse for Camera");
    ImGui::Text("Debug Menu: F1");
    ImGui::Text("%d FPS (%fms)", _fps, _frameTime);
    ImGui::End();
}

void App::ShowLightEditor()
{
    ImGui::Begin("Light Editor");

    for (int i = 0; i < scene.LightBuffer.PointLightCount; i++) {
        PointLight& light = scene.LightBuffer.PointLights[i];
        if (ImGui::TreeNode(("Light " + std::to_string(i)).c_str())) {
            ImGui::DragFloat4("Position", glm::value_ptr(light.Position));
            ImGui::ColorPicker4("Color", glm::value_ptr(light.Color));
            ImGui::DragFloat("Brightness", &light.Brightness);
            ImGui::TreePop();
        }
    }

    if (ImGui::Button("Add Light")) {
        scene.LightBuffer.PointLightCount++;
    }

    ImGui::End();
}
