/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-02-21 00:57:36
 */

#include "camera.hpp"

#include <glm/trigonometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <ImGui/imgui.h>

#include <core/log.hpp>
#include <Windows.h>

FreeCamera::FreeCamera(int width, int height)
    : _Up(0.0f, -1.0f, 1.0f),
      _Front(0.0f, 0.0f, -1.0f),
      _WorldUp(0.0f, 1.0f, 0.0f),
      _Position(0.0f, 0.0f, 1.0f),
      _Yaw(-90.0f),
      _Pitch(0.0f),
      _Friction(10.0f),
      _Acceleration(20.0f),
      _MaxVelocity(15.0f),
      _FOV(90.0f),
      _Width(width),
      _Height(height)
{
    UpdateVectors();
}

void FreeCamera::Update(double dt)
{
    GetMousePosition(_MousePos[0], _MousePos[1]);

    _View = glm::lookAt(_Position, _Position + _Front, _WorldUp);
    _Projection = glm::perspective(_FOV, float(_Width) / float(_Height), 0.05f, 10000.0f);

    UpdateVectors();
}

void FreeCamera::Input(double dt)
{
    float speedMultiplier = _Acceleration * dt;
    if (ImGui::IsKeyDown(ImGuiKey_Z)) {
        _Velocity += _Front * speedMultiplier;
    } else if (ImGui::IsKeyDown(ImGuiKey_S)) {
        _Velocity -= _Front * speedMultiplier;
    }
    if (ImGui::IsKeyDown(ImGuiKey_Q)) {
        _Velocity -= _Right * speedMultiplier;
    } else if (ImGui::IsKeyDown(ImGuiKey_D)) {
        _Velocity += _Right * speedMultiplier;
    }

    float friction_multiplier = 1.0f / (1.0f + (_Friction * dt));
    _Velocity *= friction_multiplier;

    float length = glm::length(_Velocity);
    if (length > _MaxVelocity) {
        _Velocity = glm::normalize(_Velocity) * _MaxVelocity;
    }
    _Position += _Velocity * glm::vec3(dt, dt, dt);

    int x, y;
    GetMousePosition(x, y);

    float dx = (x - _MousePos[0]) * 0.1f;
    float dy = (y - _MousePos[1]) * 0.1f;

    if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        _Yaw += dx;
        _Pitch -= dy;
    }
}

void FreeCamera::Resize(int width, int height)
{
    _Width = width;
    _Height = height;
}

void FreeCamera::UpdateVectors()
{
    glm::vec3 front;
    front.x = glm::cos(glm::radians(_Yaw)) * glm::cos(glm::radians(_Pitch));
    front.y = glm::sin(glm::radians(_Pitch));
    front.z = glm::sin(glm::radians(_Yaw)) * glm::cos(glm::radians(_Pitch));
    _Front = glm::normalize(front);
    _Right = glm::normalize(glm::cross(_Front, _WorldUp));
    _Up = glm::normalize(glm::cross(_Right, _Front));
}

void FreeCamera::GetMousePosition(int& x, int& y)
{
    POINT point;
    GetCursorPos(&point);
    x = point.x;
    y = point.y;
}
