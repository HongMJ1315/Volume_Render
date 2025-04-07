#ifndef MATRIX_H
#define MATRIX_H
#include "GLinclude.h"
#define ESP 1e-7

glm::mat3 quaternionRotate(glm::vec3, float);
glm::vec3 rotate(glm::vec3, glm::vec3, float degree);
glm::vec3 moveCameraUD(glm::vec3, glm::vec3, float);
glm::vec3 moveCameraLR(glm::vec3, glm::vec3, float);

#endif