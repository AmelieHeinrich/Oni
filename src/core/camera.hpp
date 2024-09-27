/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-31 00:14:56
 */

#pragma once

#include <glm/glm.hpp>

#include "model.hpp"

struct Plane
{
    glm::vec3 Normal = glm::vec3(0.f, 1.f, 0.f);
    float Distance = 0.f;

    Plane() = default;

    Plane(const glm::vec3& p1, const glm::vec3& norm)
		: Normal(glm::normalize(norm)),
		Distance(glm::dot(Normal, p1))
	{}

    float GetSignedDistanceToPlane(const glm::vec3& point) const
	{
		return glm::dot(Normal, point) - Distance;
	}
};

struct Frustum
{
    Plane Top;
    Plane Bottom;
    Plane Right;
    Plane Left;
    Plane Far;
    Plane Near;
};

class FreeCamera
{
public:
    FreeCamera() = default;
    FreeCamera(int width, int height);

    void Update(bool updateFrustum);
    void Input(double dt);
    void Resize(int width, int height);
    void ApplyJitter(glm::vec2 jitter);

    glm::mat4& View() { return _View; }
    glm::mat4& Projection() { return _Projection; }
    glm::vec3& GetPosition() { return _Position; }

    bool InFrustum(AABB aabb);
private:
    bool IsOnOrForwardPlane(const Plane& plane, AABB aabb);
    Frustum _Frustum;

    void UpdateVectors();
    void GetMousePosition(int& x, int& y);

    float _Yaw;
    float _Pitch;
    float _FOV;

    glm::vec3 _Position = glm::vec3(0.0f);
    glm::vec3 _Front;
    glm::vec3 _Up;
    glm::vec3 _Right;
    glm::vec3 _WorldUp;

    int _MousePos[2];
    bool _FirstMouse;

    glm::mat4 _View;
    glm::mat4 _Projection;
    glm::mat4 _ViewMat;

    float _Acceleration = 0.0f;
    float _Friction = 0.0F;
    glm::vec3 _Velocity;
    float _MaxVelocity;

    int _Width;
    int _Height;
};
