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
      _Velocity(0.0f),
      _FOV(75.0f),
      _Width(width),
      _Height(height)
{
    UpdateVectors();
}

void FreeCamera::Update(bool updateFrustum)
{
    GetMousePosition(_MousePos[0], _MousePos[1]);

    _View = glm::lookAt(_Position, _Position + _Front, _WorldUp);
    _Projection = glm::perspective(glm::radians(_FOV), float(_Width) / float(_Height), 0.05f, 10000.0f);

    UpdateVectors();

    if (updateFrustum) {
        const float halfVSide = 10000.0f * tanf(glm::radians(_FOV) * .5f);
        const float halfHSide = halfVSide * (float(_Width) / float(_Height));
        const glm::vec3 frontMultFar = 10000.0f * _Front;

        _Frustum.Near = { _Position + 0.05f * _Front, _Front };
        _Frustum.Far = { _Position + frontMultFar, -_Front };
        _Frustum.Right = { _Position, glm::cross(frontMultFar - _Right * halfHSide, _Up) };
        _Frustum.Left = { _Position, glm::cross(_Up, frontMultFar + _Right * halfHSide) };
        _Frustum.Top = { _Position, glm::cross(_Right, frontMultFar - _Up * halfVSide) };
        _Frustum.Bottom = { _Position, glm::cross(frontMultFar + _Up * halfVSide, _Right) };
    }
}

void FreeCamera::Input(double dt)
{
    float speedMultiplier = _Acceleration * dt;
    speedMultiplier *= 10.0f;
    if (ImGui::IsKeyDown(ImGuiKey_Z) || ImGui::IsKeyDown(ImGuiKey_W)) {
        _Velocity += _Front * speedMultiplier;
    } else if (ImGui::IsKeyDown(ImGuiKey_S)) {
        _Velocity -= _Front * speedMultiplier;
    }
    if (ImGui::IsKeyDown(ImGuiKey_Q) || ImGui::IsKeyDown(ImGuiKey_A)) {
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

void FreeCamera::ApplyJitter(glm::vec2 jitter)
{
    glm::mat4 haltonMatrix(
        1.0f,     0.0f,     0.0f, 0.0f,
        0.0f,     1.0f,     0.0f, 0.0f,
        0.0f,     0.0f,     1.0f, 0.0f,
        jitter.x, jitter.y, 0.0f, 1.0f
    );
    _Projection *= haltonMatrix;
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

bool FreeCamera::InFrustum(AABB aabb)
{
    return (IsOnOrForwardPlane(_Frustum.Left, aabb) &&
            IsOnOrForwardPlane(_Frustum.Right, aabb) &&
            IsOnOrForwardPlane(_Frustum.Top, aabb) &&
            IsOnOrForwardPlane(_Frustum.Bottom, aabb) &&
            IsOnOrForwardPlane(_Frustum.Near, aabb) &&
            IsOnOrForwardPlane(_Frustum.Far, aabb));
}

glm::vec4 FreeCamera::GetPlane(int i)
{
    if (i == 0) {
        return glm::vec4(_Frustum.Left.Normal, _Frustum.Left.Distance);
    }
    if (i == 1) {
        return glm::vec4(_Frustum.Right.Normal, _Frustum.Right.Distance);
    }
    if (i == 2) {
        return glm::vec4(_Frustum.Top.Normal, _Frustum.Top.Distance);
    }
    if (i == 3) {
        return glm::vec4(_Frustum.Bottom.Normal, _Frustum.Bottom.Distance);
    }
    if (i == 4) {
        return glm::vec4(_Frustum.Near.Normal, _Frustum.Near.Distance);
    }
    if (i == 5) {
        return glm::vec4(_Frustum.Far.Normal, _Frustum.Far.Distance);
    }
    return glm::vec4(1.0f);
}

bool FreeCamera::IsOnOrForwardPlane(const Plane& plane, AABB aabb)
{
    glm::vec3 vmin, vmax;

    for(int i = 0; i < 6; ++i) {
        // X axis
        if (plane.Normal.x > 0) {
            vmin.x = aabb.Min.x;
            vmax.x = aabb.Max.x;
        } else {
            vmin.x = aabb.Max.x;
            vmax.x = aabb.Min.x;
        }

        // Y axis
        if (plane.Normal.y > 0) {
            vmin.y = aabb.Min.y;
            vmax.y = aabb.Max.y;
        } else {
            vmin.y = aabb.Max.y;
            vmax.y = aabb.Min.y;
        }

        // Z axis
        if (plane.Normal.z > 0) {
            vmin.z = aabb.Min.z;
            vmax.z = aabb.Max.z;
        } else {
            vmin.z = aabb.Max.z;
            vmax.z = aabb.Min.z;
        }

        if (glm::dot(plane.Normal, vmin) + plane.Distance > 0)
            return false;
        if (glm::dot(plane.Normal, vmax) + plane.Distance >= 0)
            return true;
   }
   return true;
}
