/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-28 20:06:52
 */

#include "app.hpp"

#include "shader/bytecode.hpp"
#include "core/image.hpp"

#include "model.hpp"

#include <ImGui/imgui.h>
#include <ImGuizmo/ImGuizmo.h>

#include <glm/gtc/type_ptr.hpp>

#include <ctime>
#include <cstdlib>
#include <optick.h>

float random_float(float min, float max)
{
    float scale = rand() / (float)RAND_MAX; 
    return min + scale * ( max - min ); 
}

App::App()
    : _camera(1920, 1080), _lastFrame(0.0f)
{
    Logger::Init();
    srand(time(NULL));

    _window = std::make_shared<Window>(1920 + 16, 1080 + 39, "ONI");
    _window->OnResize([&](uint32_t width, uint32_t height) {
        _renderContext->Resize(width, height);
        _renderer->Resize(width, height);
        _camera.Resize(width, height);
    });

    _renderContext = std::make_shared<RenderContext>(_window);
    _renderer = std::make_unique<Renderer>(_renderContext);

    scene = {};

    Model sponza;
    sponza.Load(_renderContext, "assets/models/Eva01.gltf");

    scene.Models.push_back(sponza);
    scene.Lights.SetSun(glm::vec3(0.0f, -10.0f, 0.0f), glm::vec4(0.1f, 1.0f, 0.1f, 0.0f), glm::vec4(15.0f));

    _renderContext->WaitForGPU();
}

App::~App()
{
    Logger::Exit();
}

void App::Run()
{
    while (_window->IsOpen()) {
        OPTICK_FRAME("Oni");

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

        _camera.Update(dt, _updateFrustum);

        scene.Camera = _camera;

        CommandBuffer::Ptr commandBuffer = _renderContext->GetCurrentCommandBuffer();
        Texture::Ptr texture = _renderContext->GetBackBuffer();
        commandBuffer->Begin();

        // RENDER
        {
            OPTICK_EVENT("Render");
            _renderer->Render(scene, width, height, dt);
        }  

        // UI
        {
            OPTICK_EVENT("UI");
            commandBuffer->ImageBarrier(texture, TextureLayout::RenderTarget);
            commandBuffer->BindRenderTargets({ texture }, nullptr);
            commandBuffer->BeginImGui(width, height);
            RenderOverlay();
            if (!_showUI) {
                RenderHelper();
            }
            commandBuffer->EndImGui();
            commandBuffer->ImageBarrier(texture, TextureLayout::Present);
        }

        if (ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_F2)) {
            _renderer->Screenshot(texture, TextureLayout::Present);
        }

        // SUBMIT
        {
            OPTICK_EVENT("Submit");
            commandBuffer->End();
            _renderContext->ExecuteCommandBuffers({ commandBuffer }, CommandQueueType::Graphics);
        }

        // FLUSH
        {
            OPTICK_EVENT("Present");
            _renderContext->Present(_vsync);
            _renderContext->Finish();
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
                const char* vsyncLabel = _vsync ? "Disable VSync" : "Enable VSync";
                if (ImGui::MenuItem("Resource Inspector")) {
                    _showResourceInspector = !_showResourceInspector;
                }
                if (ImGui::MenuItem("Renderer Settings")) {
                    _showRendererSettings = !_showRendererSettings;
                }
                if (ImGui::MenuItem("Scene Editor")) {
                    _showLightEditor = !_showLightEditor;
                }
                if (ImGui::MenuItem(vsyncLabel)) {
                    _vsync = !_vsync;
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
    ImVec2 work_pos = viewport->WorkPos;
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
    ImGui::Text("Screenshot: F2");
    ImGui::Text("%d FPS (%fms)", _fps, _frameTime);
    ImGui::Text(_vsync ? "VSYNC: ON" : "VSYNC: OFF");
    ImGui::End();
}

void App::ShowLightEditor()
{
    ImGuiIO& io = ImGui::GetIO();

    //////////////////////////////////// IMGUIZMO

    ImGui::SetNextWindowPos({0, 0});
    ImGui::SetNextWindowSize({ 1920, 1080 });
    ImGui::Begin("ImGuizmo Context", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs);

    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

    // Draw grid
    if (_drawGrid) {
        ImGuizmo::DrawGrid(glm::value_ptr(scene.Camera.View()),
                           glm::value_ptr(scene.Camera.Projection()),
                           glm::value_ptr(glm::mat4(1.0f)),
                           100.0f);
    }

    ImGui::End();

    /////////////////////////////////// SCENE PANEL

    ImGui::Begin("Scene Editor");

    ImGui::Checkbox("Draw Grid (EXPERIMENTAL)", &_drawGrid);
    ImGui::Checkbox("Update Frustum", &_updateFrustum);

    if (ImGui::TreeNodeEx("Sun", ImGuiTreeNodeFlags_Framed)) {
        float intensity = scene.Lights.Sun.Color.x;
        ImGui::SliderFloat("Intensity", &intensity, 0.0f, 100.0f);
        scene.Lights.Sun.Color = glm::vec3(intensity);
        ImGui::TreePop();
    }

    ImGui::Separator();

    if (ImGui::Button("Add Point Light")) {
        scene.Lights.AddPointLight(PointLight(glm::vec3(0.0f), glm::vec3(1.0f), 1.0f));   
    }

    for (int i = 0; i < scene.Lights.PointLights.size(); i++) {
        auto& light = scene.Lights.PointLights[i];
        if (ImGui::TreeNodeEx(std::string("Point Light " + std::to_string(i)).c_str(), ImGuiTreeNodeFlags_Framed)) {
            ImGui::ColorPicker3("Color", glm::value_ptr(light.Color), ImGuiColorEditFlags_PickerHueBar);
            ImGui::SliderFloat("Brightness", &light.Brightness, 0.0f, 100.0f);

            glm::mat4 Transform(1.0f);
            glm::vec3 DummyRotation;
            glm::vec3 DummyScale;

            ImGuizmo::RecomposeMatrixFromComponents(glm::value_ptr(light.Position),
                                                    glm::value_ptr(glm::vec3(0.0f)), 
                                                    glm::value_ptr(glm::vec3(1.0f)),
                                                    glm::value_ptr(Transform));

            ImGuizmo::Manipulate(glm::value_ptr(scene.Camera.View()),
                                 glm::value_ptr(scene.Camera.Projection()),
                                 ImGuizmo::OPERATION::TRANSLATE,
                                 ImGuizmo::MODE::WORLD,
                                 glm::value_ptr(Transform),
                                 nullptr,
                                 nullptr);

            ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(Transform),
                                                  glm::value_ptr(light.Position),
                                                  glm::value_ptr(DummyRotation),
                                                  glm::value_ptr(DummyScale));

            ImGui::TreePop();
        }
    }

    ImGui::End();
}
