//
// Created by lasagnaphil on 2021-08-02.
//

#ifndef THESYSTEM_CAMERA_H
#define THESYSTEM_CAMERA_H

#include "core/types.h"

#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    Camera(int screen_width, int screen_height) {
        proj_mat = glm::ortho(0.f, (float)screen_width, (float)screen_height, 0.f, -100.0f, 100.0f);
        model_mat = mat4(1.0f);
    }

    mat4 get_proj_mat() const { return proj_mat; }
    mat4 get_model_mat() const { return model_mat; }

private:
    mat4 proj_mat;
    mat4 model_mat;
};

#endif //THESYSTEM_CAMERA_H
