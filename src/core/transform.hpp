//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-15 15:48:18
//

#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <ImGui/imgui.h>
#include <ImGuizmo/ImGuizmo.h>

class Transform
{
public:
    Transform()
        : Matrix(1.0f), Position(0.0f), Rotation(0.0f), Scale(1.0f)
    {}
    ~Transform() = default;

    glm::vec3 GetFrontVector();
    void Update();

    glm::vec3 Position = glm::vec3(0.0f);
    glm::vec3 Rotation = glm::vec3(0.0f);
    glm::vec3 Scale = glm::vec3(1.0f);
    glm::mat4 Matrix = glm::mat4(1.0f);
};
