#include "matrix.h"

glm::mat3 quaternionRotate(glm::vec3 axis, float theta){
    axis = glm::normalize(axis);
    float a = cos(theta / 2.0f);
    float b = -axis.x * sin(theta / 2.0f);
    float c = -axis.y * sin(theta / 2.0f);
    float d = -axis.z * sin(theta / 2.0f);
    float aa = a * a, bb = b * b, cc = c * c, dd = d * d;
    float bc = b * c, ad = a * d, ac = a * c, ab = a * b, bd = b * d, cd = c * d;
    glm::mat3 rot_mat;
    rot_mat[0][0] = aa + bb - cc - dd;
    rot_mat[0][1] = 2 * (bc + ad);
    rot_mat[0][2] = 2 * (bd - ac);
    rot_mat[1][0] = 2 * (bc - ad);
    rot_mat[1][1] = aa + cc - bb - dd;
    rot_mat[1][2] = 2 * (cd + ab);
    rot_mat[2][0] = 2 * (bd + ac);
    rot_mat[2][1] = 2 * (cd - ab);
    rot_mat[2][2] = aa + dd - bb - cc;
    return rot_mat;
}

glm::vec3 rotate(glm::vec3 O, glm::vec3 P, float degree){
    float t = degree * glm::pi<float>() / 180.0f;

    glm::vec3 OP = P - O;
    glm::vec3 Z(0, 0, 1);

    glm::vec3 OM(0, 1, 0);

    glm::vec3 axis = glm::cross(OP, OM);

    axis = glm::normalize(axis);

    glm::mat3 R = quaternionRotate(axis, t);

    glm::vec3 P_prime = R * OP + O;
    return P_prime;
}

glm::vec3 moveCameraUD(glm::vec3 O, glm::vec3 P, float degree){
    glm::vec3 result = rotate(O, P, degree);
    glm::vec3 tresult = result - O;
    glm::vec3 tv(0, 1, 0);
    float cos_theta = glm::dot(tresult, tv) / (glm::length(tresult) * glm::length(tv));
    float angle = acos(cos_theta) * 180.0f / glm::pi<float>();

    if(angle < ESP || 180.0f - angle < ESP)
        return P;
    else
        return result;
}

glm::vec3 moveCameraLR(glm::vec3 O, glm::vec3 P, float degree){
    //旋轉中心
    float center_x = O.x, center_z = O.z;
    //移回中心
    float tox = P.x - center_x, toz = P.z - center_z;

    degree = degree * glm::pi<float>() / 180.0f;
    //旋轉
    float tx = tox * cos(degree) - toz * sin(degree),
        tz = tox * sin(degree) + toz * cos(degree);
    //移回
    return glm::vec3(tx + center_x, P.y, tz + center_z);
}