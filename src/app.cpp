//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-03-28 20:06:52
//

#include <ImGui/imgui.h>
#include <ImGuizmo/ImGuizmo.h>

#include <glm/gtc/type_ptr.hpp>

#include <optick.h>

#include <ctime>
#include <cstdlib>
#include <sstream>
#include <iomanip>

#include "app.hpp"

#include "core/shader_bytecode.hpp"
#include "core/bitmap.hpp"
#include "core/texture_compressor.hpp"
#include "core/file_system.hpp"
#include "core/model.hpp"
#include "core/shader_loader.hpp"
#include "core/util.hpp"

#include "renderer/techniques/debug_renderer.hpp"

#define SCENE_BALLS 0
#define SCENE_SPONZA 0
#define SCENE_BISTRO 1
#define SCENE_EMERALDSQUARE 0
#define SCENE_SUNTEMPLE 0
#define SCENE_SMALL 0
#define SCENE_TEXTURE_COMPRESSION_TEST 0
#define SCENE_PLATFORM 0

constexpr int TEST_LIGHT_COUNT = 0;

App::App()
    : _camera(1920, 1080), _lastFrame(0.0f)
{
    Logger::Init();

    // Initializes engine directories if needed
    if (!FileSystem::Exists("screenshots")) {
        FileSystem::CreateDirectoryFromPath("screenshots");
    }
    if (!FileSystem::Exists("screenshots/engine")) {
        FileSystem::CreateDirectoryFromPath("screenshots/engine");
    }

    // Compress every model texture
    TextureCompressor::TraverseDirectory("assets/", TextureCompressorFormat::BC7);

    // Load/Cache every shader
    ShaderLoader::TraverseDirectory("shaders/");

    // Make window
    _window = std::make_shared<Window>(1920, 1080, "ONI");
    _window->OnResize([&](uint32_t width, uint32_t height) {
        _renderContext->Resize(width, height);
        _renderer->Resize(width, height);
        _camera.Resize(width, height);
    });

    // Create render context and renderer
    _renderContext = std::make_shared<RenderContext>(_window);
    _renderer = std::make_unique<Renderer>(_renderContext);

    // Push models and lights
    SetupScene();

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

        _frameTimer.Restart();
        auto& stats = _renderer->GetStats();

        static float framesPerSecond = 0.0f;
        static float lastTime = 0.0f;
        float currentTime = GetTickCount() * 0.001f;
        ++framesPerSecond;
        if (currentTime - lastTime > 1.0f) {
            lastTime = currentTime;
            _fps = (int)framesPerSecond;
            framesPerSecond = 0;
        }
        _pastFps.push_back(_fps);
        if (_pastFps.size() > _historySize) {
            _pastFps.erase(_pastFps.begin());
        }

        float time = _dtTimer.GetElapsed();
        float dt = (time - _lastFrame) / 1000.0f;
        _lastFrame = time;

        _camera.Update(_updateFrustum);

        if (ImGui::IsKeyPressed(ImGuiKey_F1)) {
            _showUI = !_showUI;
        }

        uint32_t width, height;
        _window->Update();
        _window->GetSize(width, height);

        scene.Camera = _camera;

        scene.Lights.Sun.Direction = scene.Lights.SunTransform.GetFrontVector();

        glm::vec3 A = scene.Lights.SunTransform.Position;
        glm::vec3 B = A + scene.Lights.SunTransform.GetFrontVector();

        DebugRenderer::Get()->PushLine(B, A, glm::vec3(1.0f));

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
            stats.PushFrameTime("UI", [this, commandBuffer, texture, width, height]() {
                commandBuffer->BeginEvent("ImGui");
                commandBuffer->ImageBarrier(texture, TextureLayout::RenderTarget);
                commandBuffer->BindRenderTargets({ texture }, nullptr);
                commandBuffer->BeginImGui(width, height);
                RenderOverlay();
                if (!_showUI) {
                    RenderHelper();
                }
                commandBuffer->EndImGui();
                commandBuffer->ImageBarrier(texture, TextureLayout::Present);
                commandBuffer->EndEvent();
            });
        }

        if (ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_F2)) {
            _renderer->Screenshot(texture, TextureLayout::Present);
        }
        if (ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_F3)) {
            _hideOverlay = !_hideOverlay;
        }

        // SUBMIT
        {
            OPTICK_EVENT("Submit");
            commandBuffer->End();

            stats.PushFrameTime("Submit", [this, commandBuffer] {
                _renderContext->ExecuteCommandBuffers({ commandBuffer }, CommandQueueType::Graphics);
            });
        }

        // FLUSH
        {
            OPTICK_EVENT("Present");

            stats.PushFrameTime("Present", [this]() {
                _renderContext->Present(_vsync);
                _renderContext->Finish();
            });
        }

        // Update matrices
        {
            for (auto& model : scene.Models) {
                for (auto& primitive : model.Primitives) {
                    primitive.PrevTransform = primitive.Transform;
                }
            }
            scene.PrevViewProj = scene.Camera.Projection() * scene.Camera.View();
        }

        _renderer->Reconstruct();

        DebugRenderer::Get()->Reset();

        if (!_showUI) {
            _camera.Input(dt);
        }

        if ((_updateTimer.GetElapsed() / 1000.0f) > 1.0f) {
            _frameTime = _frameTimer.GetElapsed();
            _updateTimer.Restart();
        } 

        _frameCount++;
    }
}

void App::RenderOverlay()
{
    if (_showUI) {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Window")) {
                const char* vsyncLabel = _vsync ? "Disable VSync" : "Enable VSync";
                if (ImGui::MenuItem(vsyncLabel)) {
                    _vsync = !_vsync;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Scene")) {
                if (ImGui::MenuItem("Scene Editor")) {
                    _showLightEditor = !_showLightEditor;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Debug")) {
                if (ImGui::MenuItem("Log")) {
                    _showLogger = !_showLogger;
                }
                if (ImGui::MenuItem("Resource Inspector")) {
                    _showResourceInspector = !_showResourceInspector;
                }
                if (ImGui::MenuItem("Renderer Settings")) {
                    _showRendererSettings = !_showRendererSettings;
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
        if (_showLogger) {
            Logger::OnUI();
        }

        _renderContext->OnOverlay();
    }
}

void App::RenderHelper()
{
    if (!_hideOverlay) {
        static bool p_open = true;
        auto stats = _renderer->GetStats();

        ImGuiIO& io = ImGui::GetIO();
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDocking;
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

        ImGui::SetNextWindowBgAlpha(0.70f);
        ImGui::Begin("Example: Simple overlay", &p_open, window_flags);
        ImGui::Text("WASD + Mouse for Camera");
        ImGui::Text("Debug Menu: F1");
        ImGui::Text("Screenshot: F2");
        ImGui::Text("Hide Overlay: F3");
        ImGui::Separator();
        ImGui::Text(_vsync ? "VSYNC: ON" : "VSYNC: OFF");
        ImGui::Text("%d FPS (%.2fms)", _fps, _frameTime);
        ImGui::Separator();
        for (auto pair : stats.FrameTimesHistory) {
            char buffer[256] = {};
            sprintf(buffer, "%s (%.2fms)", pair.first.c_str(), pair.second.back());
            ImGui::PlotLines(buffer, pair.second.data(), pair.second.size());
        }
        ImGui::End();
    }
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

    static ImGuizmo::OPERATION operation = ImGuizmo::OPERATION::ROTATE;

    ImGui::Begin("Scene Editor");

    ImGui::Checkbox("Draw Grid (EXPERIMENTAL)", &_drawGrid);
    ImGui::Checkbox("Update Frustum", &_updateFrustum);

    if (ImGui::TreeNodeEx("Lights", ImGuiTreeNodeFlags_Framed)) {
        if (ImGui::TreeNodeEx("Sun", ImGuiTreeNodeFlags_Framed)) {
            float intensity = scene.Lights.Sun.Color.x;
            ImGui::SliderFloat("Intensity", &intensity, 0.0f, 100.0f);
            scene.Lights.Sun.Color = glm::vec3(intensity);

            ImGui::SliderFloat3("Position", glm::value_ptr(scene.Lights.SunTransform.Position), -100.0f, 100.0f);
            ImGui::SliderFloat3("Rotation", glm::value_ptr(scene.Lights.SunTransform.Rotation), -360.0f, 360.0f);

            if (ImGui::Button("Translate")) {
                operation = ImGuizmo::OPERATION::TRANSLATE;
            }
            ImGui::SameLine();
            if (ImGui::Button("Rotate")) {
                operation = ImGuizmo::OPERATION::ROTATE;
            }

            ImGuizmo::RecomposeMatrixFromComponents(glm::value_ptr(scene.Lights.SunTransform.Position),
                                                    glm::value_ptr(scene.Lights.SunTransform.Rotation),
                                                    glm::value_ptr(scene.Lights.SunTransform.Scale),
                                                    glm::value_ptr(scene.Lights.SunTransform.Matrix));

            ImGuizmo::Manipulate(glm::value_ptr(scene.Camera.View()),
                                 glm::value_ptr(scene.Camera.Projection()),
                                 operation,
                                 ImGuizmo::MODE::WORLD,
                                 glm::value_ptr(scene.Lights.SunTransform.Matrix),
                                 nullptr,
                                 nullptr);

            ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(scene.Lights.SunTransform.Matrix),
                                                  glm::value_ptr(scene.Lights.SunTransform.Position),
                                                  glm::value_ptr(scene.Lights.SunTransform.Rotation),
                                                  glm::value_ptr(scene.Lights.SunTransform.Scale));

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
        
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("Game Objects", ImGuiTreeNodeFlags_Framed)) {
        for (auto& model : scene.Models) {
            if (ImGui::TreeNodeEx(model.Name.c_str(), ImGuiTreeNodeFlags_Framed)) {
                for (auto& primitive : model.Primitives) {
                    if (ImGui::TreeNodeEx(primitive.Name.c_str(), ImGuiTreeNodeFlags_Framed)) {
                        ImGui::TreePop();
                    }
                }
                ImGui::TreePop();
            }
        }
        ImGui::TreePop();
    }

    ImGui::End();
}

void App::SetupScene()
{
    scene = {};

#if SCENE_SMALL
    Model platform = {};
    platform.Load(_renderContext, "assets/models/platform/Platform.gltf");

    Model damagedHelmet = {};
    damagedHelmet.Load(_renderContext, "assets/models/flighthelmet/FlightHelmet.gltf");

    Model scifiHelmet = {};
    scifiHelmet.Load(_renderContext, "assets/models/scifi/SciFiHelmet.gltf");
    scifiHelmet.ApplyTransform(glm::translate(glm::mat4(1.0f), glm::vec3(-3.0f, 0.0f, 0.0f)));

    Model suzanne = {};
    suzanne.Load(_renderContext, "assets/models/suzanne/Suzanne.gltf");
    suzanne.ApplyTransform(glm::translate(glm::mat4(1.0f), glm::vec3(3.0f, 0.0f, 0.0f)));

    scene.Models.push_back(platform);
    scene.Models.push_back(damagedHelmet);
    scene.Models.push_back(scifiHelmet);
    scene.Models.push_back(suzanne);

    scene.Lights.SetSun(glm::vec3(0.0f, 20.0f, 0.0f), glm::vec3(-90.0f, 0.0f, 0.0f), glm::vec4(5.0f));
#endif

#if SCENE_SPONZA
    Model sponza = {};
    sponza.Load(_renderContext, "assets/models/sponza/Sponza.gltf");

    scene.Models.push_back(sponza);
    scene.Lights.SetSun(glm::vec3(0.0f, 20.0f, 0.0f), glm::vec3(-90.0f, 0.0f, 17.0f), glm::vec4(5.0f));
#endif

#if SCENE_BALLS
    Model balls = {};
    balls.Load(_renderContext, "assets/models/balls/MetalRoughSpheres.gltf");

    scene.Models.push_back(balls);
    scene.Lights.SetSun(glm::vec3(0.0f, 20.0f, 0.0f), glm::vec3(-90.0f, 0.0f, 0.0f), glm::vec4(5.0f));
#endif

#if SCENE_TEXTURE_COMPRESSION_TEST
    Model scifiHelmet = {};
    scifiHelmet.Load(_renderContext, "assets/models/scifi/SciFiHelmet.gltf");
    
    scene.Models.push_back(scifiHelmet);
    scene.Lights.SetSun(glm::vec3(0.0f, 20.0f, 0.0f), glm::vec3(-90.0f, 0.0f, 0.0f), glm::vec4(5.0f));
#endif

#if SCENE_BISTRO
    Model bistro = {};
    bistro.Load(_renderContext, "assets/models/bistro/bistro.gltf");
    scene.Lights.SetSun(glm::vec3(0.0f, 30.0f, 0.0f), glm::vec3(-90.0f, 30.0f, 0.0f), glm::vec4(5.0f));

    scene.Models.push_back(bistro);
#endif

#if SCENE_EMERALDSQUARE
    Model bistro = {};
    bistro.Load(_renderContext, "assets/models/emeraldsquare/EmeraldSquare.gltf");
    scene.Lights.SetSun(glm::vec3(0.0f, 30.0f, 0.0f), glm::vec3(-90.0f, 30.0f, 0.0f), glm::vec4(5.0f));

    scene.Models.push_back(bistro);
#endif

#if SCENE_SUNTEMPLE
    Model bistro = {};
    bistro.Load(_renderContext, "assets/models/suntemple/SunTemple.gltf");
    scene.Lights.SetSun(glm::vec3(0.0f, 30.0f, 0.0f), glm::vec3(-90.0f, 30.0f, 0.0f), glm::vec4(5.0f));

    scene.Models.push_back(bistro);
#endif

#if SCENE_PLATFORM
    Model bistro = {};
    bistro.Load(_renderContext, "assets/models/platform/Platform.gltf");
    scene.Lights.SetSun(glm::vec3(0.0f, 30.0f, 0.0f), glm::vec3(-90.0f, 30.0f, 0.0f), glm::vec4(5.0f));

    scene.Models.push_back(bistro);
#endif

    for (int i = 0; i < TEST_LIGHT_COUNT; i++) {
        scene.Lights.AddPointLight(PointLight(
            glm::vec3(util::random_range(-6.0f, 6.0f), util::random_range(1.0f, 8.0f), util::random_range(-6.0f, 6.0f)),
            glm::vec3(util::random_range(0.0f, 1.0f),  util::random_range(0.0f, 1.0f), util::random_range(0.0f, 1.0f)),
            1.0f
        ));
    }
}
