/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-31 00:14:56
 */

#pragma once

#include <glm/glm.hpp>

class FreeCamera
{
public:
    FreeCamera(int width, int height);

    void Update(double dt);
    void Input(double dt);
    void Resize(int width, int height);

    glm::mat4 View() { return _View; }
    glm::mat4 Projection() { return _Projection; }
private:
    void UpdateVectors();
    void GetMousePosition(int& x, int& y);

    float _Yaw;
    float _Pitch;
    float _FOV;

    glm::vec3 _Position;
    glm::vec3 _Front;
    glm::vec3 _Up;
    glm::vec3 _Right;
    glm::vec3 _WorldUp;

    int _MousePos[2];
    bool _FirstMouse;

    glm::mat4 _View;
    glm::mat4 _Projection;
    glm::mat4 _ViewMat;

    float _Acceleration;
    float _Friction;
    glm::vec3 _Velocity;
    float _MaxVelocity;

    int _Width;
    int _Height;
};
