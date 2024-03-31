/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-31 15:35:56
 */

#pragma once

#include "model.hpp"

#include <glm/gtc/type_ptr.hpp>

struct Scene
{
    std::vector<Model> Models;
    
    glm::mat4 View;
    glm::mat4 Projection;
    
    // TODO(ahi): Frustum planes
};
