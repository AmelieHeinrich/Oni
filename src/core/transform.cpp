//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-15 15:58:06
//

#include "transform.hpp"

glm::vec3 Transform::GetFrontVector()
{
    glm::vec3 front;
    front.x = Matrix[2][0];
    front.y = Matrix[2][1];
    front.z = Matrix[2][2];
    return front;
}

void Transform::Update()
{
    ImGuizmo::RecomposeMatrixFromComponents(glm::value_ptr(Position),
                                            glm::value_ptr(Rotation),
                                            glm::value_ptr(Scale),
                                            glm::value_ptr(Matrix));
}
