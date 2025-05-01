#include "ui.h"

#define PIXEL_SIZE 1
void line_editor_winodw(int max_density, std::vector<float> &tf_r, std::vector<float> &tf_g, std::vector<float> &tf_b, std::vector<float> &tf_alp){
    // 建立一個視窗
    ImGui::Begin("RGB Transfer Function");

    static int currentChannel = 0; // 0=red, 1=green, 2=blue
    ImGui::RadioButton("Red", &currentChannel, 0); ImGui::SameLine();
    ImGui::RadioButton("Green", &currentChannel, 1); ImGui::SameLine();
    ImGui::RadioButton("Blue", &currentChannel, 2); ImGui::SameLine();
    ImGui::RadioButton("Alpha", &currentChannel, 3);

    ImVec2 canvasSize(400, 250);
    ImGui::InvisibleButton("canvas", canvasSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
    ImVec2 canvasPos = ImGui::GetItemRectMin();
    ImVec2 canvasEnd = ImGui::GetItemRectMax();
    ImDrawList *drawList = ImGui::GetWindowDrawList();

    drawList->AddRectFilled(canvasPos, canvasEnd, IM_COL32(50, 50, 50, 255));
    const float gridStepX = canvasSize.x / 10.0f;
    const float gridStepY = canvasSize.y / 10.0f;
    for(float x = canvasPos.x; x <= canvasEnd.x; x += gridStepX)
        drawList->AddLine(ImVec2(x, canvasPos.y), ImVec2(x, canvasEnd.y), IM_COL32(100, 100, 100, 40));
    for(float y = canvasPos.y; y <= canvasEnd.y; y += gridStepY)
        drawList->AddLine(ImVec2(canvasPos.x, y), ImVec2(canvasEnd.x, y), IM_COL32(100, 100, 100, 40));

    auto toCanvas = [&](float intensity, float val){
        int x = canvasPos.x + (intensity / 255.0f) * canvasSize.x;
        float y = canvasPos.y + canvasSize.y - (val * canvasSize.y);
        return ImVec2(x, y);
    };


    struct ChannelData{
        std::vector<ChannelPoint> *pts;
        ImU32 color;
    };
    ChannelData channels[4] = {
        { &redPoints,   IM_COL32(255, 0, 0, 255) },
        { &greenPoints, IM_COL32(0, 255, 0, 255) },
        { &bluePoints,  IM_COL32(0, 0, 255, 255) },
        { &alphaPoints,  IM_COL32(150, 150, 150, 255) }
    };

    auto buildPolyline = [&](ChannelData &ch){
        auto &vec = *(ch.pts);
        std::sort(vec.begin(), vec.end(), [](auto &a, auto &b){
            return a.intensity < b.intensity;
        });
        std::vector<ImVec2> linePts;
        linePts.reserve(vec.size());
        for(auto &p : vec){
            linePts.push_back(toCanvas(p.intensity, p.value));
        }
        return linePts;
    };

    for(int c = 0; c < 4; c++){
        auto linePts = buildPolyline(channels[c]);
        if(linePts.size() > 1){
            drawList->AddPolyline(linePts.data(), (int) linePts.size(), channels[c].color, false, 2.0f);
        }
    }



    static int activeChannel = -1; // -1 表示目前沒有拖曳任何點
    static int activeIndex = -1;
    static bool dragging = false;

    bool canvasHovered = ImGui::IsItemHovered();
    ImVec2 mousePos = ImGui::GetIO().MousePos;
    float minDist = 1e6f;
    int nearestChannel = -1;
    int nearestIndex = -1;

    if(canvasHovered){
        const float hitThreshold = 10.0f; // 感應半徑
        for(int c = 0; c < 4; c++){
            auto &pts = *(channels[c].pts);
            for(int i = 0; i < (int) pts.size(); i++){
                ImVec2 pCanvas = toCanvas(pts[i].intensity, pts[i].value);
                float dx = mousePos.x - pCanvas.x;
                float dy = mousePos.y - pCanvas.y;
                float dist = std::sqrt(dx * dx + dy * dy);
                if(dist < hitThreshold && dist < minDist){
                    minDist = dist;
                    nearestChannel = c;
                    nearestIndex = i;
                }
            }
        }
    }

    if(canvasHovered && ImGui::IsMouseReleased(ImGuiMouseButton_Right)){
        if(nearestChannel != -1 && nearestIndex != -1){
            auto &pts = *(channels[nearestChannel].pts);
            if(nearestIndex != 0 && nearestIndex != (int) pts.size() - 1){
                pts.erase(pts.begin() + nearestIndex);
            }
        }
    }

    if(canvasHovered && ImGui::IsMouseDown(ImGuiMouseButton_Left)){
        if(!dragging){
            if(nearestChannel != -1 && nearestIndex != -1){
                activeChannel = nearestChannel;
                activeIndex = nearestIndex;
                dragging = true;
            }
        }
        else{
            auto &pts = *(channels[activeChannel].pts);
            if(activeIndex >= 0 && activeIndex < (int) pts.size()){
                float newIntensity = (mousePos.x - canvasPos.x) / canvasSize.x * 255.0f;
                float newValue = (canvasEnd.y - mousePos.y) / canvasSize.y;
                newIntensity = std::clamp(newIntensity, 0.0f, 255.0f);
                newValue = std::clamp(newValue, 0.0f, 1.0f);
                if(activeIndex == 0){
                    newIntensity = 0.0f;
                }
                else if(activeIndex == (int) pts.size() - 1){
                    newIntensity = 255.0f;
                }
                pts[activeIndex].intensity = int(newIntensity);
                pts[activeIndex].value = newValue;
            }
        }
    }
    else{
        dragging = false;
        activeChannel = -1;
        activeIndex = -1;
    }


    if(canvasHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)){
        if(nearestChannel == -1 || nearestIndex == -1){
            auto &pts = *(channels[currentChannel].pts);
            float newIntensity = (mousePos.x - canvasPos.x) / canvasSize.x * 255.0f;
            float newValue = (canvasEnd.y - mousePos.y) / canvasSize.y;
            newIntensity = std::clamp(newIntensity, 0.0f, 255.0f);
            newValue = std::clamp(newValue, 0.0f, 1.0f);
            pts.push_back({ int(newIntensity), newValue });
        }
    }


    const float pointRadius = 4.0f;
    for(int c = 0; c < 4; c++){
        auto &pts = *(channels[c].pts);
        ImU32 col = channels[c].color;
        for(int i = 0; i < (int) pts.size(); i++){
            ImVec2 pCanvas = toCanvas(pts[i].intensity, pts[i].value);
            drawList->AddCircleFilled(pCanvas, pointRadius, col);
        }
    }


    // if(ImGui::Button("Get Vector")){
    auto processChannel = [&](const std::vector<ChannelPoint> &pts_in, const char *name){
        // std::ofstream ofs(std::string(name) + ".txt", std::ofstream::out);

        // 1. 排序并确保 0 / 255 两端点
        std::vector<ChannelPoint> pts = pts_in;
        std::sort(pts.begin(), pts.end(), [](auto &a, auto &b){
            return a.intensity < b.intensity;
        });
        if(pts.empty() || pts.front().intensity != 0)
            pts.insert(pts.begin(), { 0, pts.empty() ? 0.0f : pts.front().value });
        if(pts.back().intensity != 255)
            pts.push_back({ 255, pts.back().value });

        // std::cout << name << " control points:\n";
        // for(auto &p : pts)
        //     std::cout << "(" << p.intensity << "," << p.value << ") ";
        // std::cout << "\n";

        std::vector<float> tf(256);
        int seg = 0;
        for(int i = 0; i < 256; ++i){
            while(seg + 1 < (int) pts.size() && i > pts[seg + 1].intensity)
                ++seg;
            int   x0 = pts[seg].intensity, x1 = pts[seg + 1].intensity;
            float y0 = pts[seg].value, y1 = pts[seg + 1].value;
            float t = (x1 == x0) ? 0.0f : float(i - x0) / float(x1 - x0);
            float v = (1.0f - t) * y0 + t * y1;
            tf[i] = v;    
        }

        // ofs << 256 << "\n";
        // for(float f : tf) ofs << f << " ";
        // ofs << std::endl;

        return tf;
    };

    tf_r = processChannel(redPoints, "Red");
    tf_g = processChannel(greenPoints, "Green");
    tf_b = processChannel(bluePoints, "Blue");
    tf_alp = processChannel(alphaPoints, "Alpha");
    // }

    // ... 后面不变


    ImGui::Dummy(canvasSize);

    ImGui::End();
}

void input_window(glm::vec3 &camera_pos, glm::vec3 &camera_front, Volume &volume, std::vector<unsigned char> &data, int &m, int &k, int &threadhold, float &gamma, int &cell_size){
    ImGui::Begin("Input Window");
    ImGui::Text("Camera Position: (%.2f, %.2f, %.2f)", camera_pos.x, camera_pos.y, camera_pos.z);
    ImGui::Text("Camera Front Position: (%.2f, %.2f, %.2f)", camera_front.x, camera_front.y, camera_front.z);
    std::vector<int> d = volume.get_distribute();
    std::vector<float> fd;
    for(auto i : d) fd.push_back(i);
    ImVec2 graph_size = ImVec2(0, 80);
    ImGui::PlotHistogram("Density distribution", fd.data(), fd.size(), 0, nullptr, FLT_MAX, FLT_MAX, graph_size);
    ImGui::InputInt("M", &m);
    ImGui::InputInt("K", &k);
    if(ImGui::Button("Compute Histogram2D")){
        volume.compute_histogram2d(m, k);
    }
    ImGui::InputFloat("Gamma", &gamma);
    ImGui::InputInt("Threadhold", &threadhold);
    ImGui::InputInt("Cell Size", &cell_size);
    if(ImGui::Button("Reset Data")){
        volume = Volume(data, volume.length, volume.width, volume.height, gamma, threadhold);
        m = volume.get_distribute().size() - 1;
        volume.compute_gradient(1.0f, 255.0f);
        volume.compute_histogram2d(m, k);
    }

    ImGui::End();
}


void histogram_window(Volume &volume, int &cell){
    ImGui::Begin("Histogram2D Viewer");
    std::vector<std::vector<int>> histogram2D = volume.get_histogram2d();

    const int cell_size = cell;
    const int M = histogram2D.size();
    const int K = histogram2D[0].size();

    int maxFreq = 0;
    for(int i = 0; i < M; ++i)
        for(int j = 0; j < K; ++j)
            maxFreq = std::max(maxFreq, histogram2D[i][j]);

    ImVec2 origin = ImGui::GetCursorScreenPos();
    ImDrawList *draw_list = ImGui::GetWindowDrawList();

    for(int i = 0; i < M; ++i){
        for(int j = 0; j < K; ++j){
            float freq = (float) histogram2D[i][j];
            float norm = freq / maxFreq; // 正規化為 0~1
            int gray = 255 - int(norm * 255.0f);
            ImU32 color = IM_COL32(gray, gray, gray, 255);

            ImVec2 p_min = ImVec2(origin.x + j * cell_size, origin.y + i * cell_size);
            ImVec2 p_max = ImVec2(p_min.x + cell_size, p_min.y + cell_size);
            draw_list->AddRectFilled(p_min, p_max, color);
        }
    }

    ImVec2 nxt_histogram_pos_x = ImVec2(origin.x, origin.y + M * cell_size + 10);
    ImVec2 nxt_histogram_pos_y = ImVec2(origin.x + K * cell_size + 10, origin.y);
    std::vector<float> histogram1D_y(K, 0.0f);
    std::vector<float> histogram1D_x(M, 0.0f);
    for(int i = 0; i < M; ++i){
        for(int j = 0; j < K; ++j){
            histogram1D_x[i] += histogram2D[i][j];
            histogram1D_y[j] += histogram2D[i][j];
        }
    }
    float min_val_x = FLT_MAX, max_val_x = -FLT_MAX;
    float min_val_y = FLT_MAX, max_val_y = -FLT_MAX;
    for(int i = 0; i < K; ++i){
        min_val_y = std::min(min_val_y, histogram1D_y[i]);
        max_val_y = std::max(max_val_y, histogram1D_y[i]);
    }
    for(int i = 0; i < M; ++i){
        min_val_x = std::min(min_val_x, histogram1D_x[i]);
        max_val_x = std::max(max_val_x, histogram1D_x[i]);
    }
    for(int i = 0; i < K; ++i){
        float freq = histogram1D_y[i];
        float norm = (freq - min_val_y) / (max_val_y - min_val_y); // 正規化為 0~1
        int gray = 255 - int(norm * 255.0f);
        ImU32 color = IM_COL32(gray, gray, gray, 255);

        ImVec2 p_min = ImVec2(nxt_histogram_pos_x.x + i * cell_size, nxt_histogram_pos_x.y);
        ImVec2 p_max = ImVec2(p_min.x + cell_size, nxt_histogram_pos_x.y + cell_size * 5);
        draw_list->AddRectFilled(p_min, p_max, color);
    }
    for(int i = 0; i < M; ++i){
        float freq = histogram1D_x[i];
        float norm = (freq - min_val_x) / (max_val_x - min_val_x); // 正規化為 0~1
        int gray = 255 - int(norm * 255.0f);
        ImU32 color = IM_COL32(gray, gray, gray, 255);

        ImVec2 p_min = ImVec2(nxt_histogram_pos_y.x, nxt_histogram_pos_y.y + i * cell_size);
        ImVec2 p_max = ImVec2(nxt_histogram_pos_y.x + cell_size * 5, p_min.y + cell_size);
        draw_list->AddRectFilled(p_min, p_max, color);
    }


    ImGui::Dummy(ImVec2(M * cell_size, M * cell_size + 10));

    ImGui::End();
}
