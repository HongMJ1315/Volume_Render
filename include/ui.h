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

// 單一「通道」的折點
struct ChannelPoint{
    float intensity; // X 軸，範圍 [0, 255]
    float value;     // Y 軸，範圍 [0, 1]
};

// 三條線的控制點，全域或放在別處皆可
static std::vector<ChannelPoint> redPoints = {
    {0.0f, 0.0f},
    {255.0f, 0.0f}
};
static std::vector<ChannelPoint> greenPoints = {
    {0.0f, 0.0f},
    {255.0f, 0.0f}
};
static std::vector<ChannelPoint> bluePoints = {
    {0.0f, 0.0f},
    {255.0f, 0.0f}
};

void line_editor_winodw();
void input_window(glm::vec3 &camera_pos, glm::vec3 &camera_front, Volume &volume, int &m, int &k);
void histogram_window(Volume &volume);

#endif // UI_H