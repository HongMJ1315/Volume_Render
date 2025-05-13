#ifndef UI_H
#define UI_H

#include "GLinclude.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "volume.h"

#include <vector>
#include <algorithm>
#include <cmath>
#include <utility>
#include <iostream>
#include <fstream>      
struct ChannelPoint{
    int intensity;
    float value;
};

static std::vector<ChannelPoint> redPoints = {
    {0, 0.0f},
    {255, 0.0f}
};
static std::vector<ChannelPoint> greenPoints = {
    {0, 0.0f},
    // {128, 0.0f},
    {255, 0.0f}
};
static std::vector<ChannelPoint> bluePoints = {
    {0, 0.0f},
    {255, 0.0f}
};
static std::vector<ChannelPoint> alphaPoints = {
    {0, 0.0f},
    {255, 0.0f}
};
// void line_editor_winodw(int max_density, std::vector<float> &, std::vector<float> &, std::vector<float> &, std::vector<float> &);
void input_window(glm::vec3 &camera_pos, glm::vec3 &camera_front,
    Volume &volume, std::vector<unsigned char> &data,
    int &m, int &k, int &threadhold, float &gamma, int &cell_size, int &is_phong);
// void histogram_window(Volume &volume, int &cell_size);
void HistogramTFEditor(Volume &volume,
    std::vector<float> &tf_r,
    std::vector<float> &tf_g,
    std::vector<float> &tf_b,
    std::vector<float> &tf_alp,
    int cell_size);
#endif // UI_H