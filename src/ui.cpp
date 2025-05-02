#include "ui.h"
#include <numeric>

#define PIXEL_SIZE 1

/*
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

    ImGui::Dummy(canvasSize);

    ImGui::End();
}
//*/

void input_window(glm::vec3 &camera_pos, glm::vec3 &camera_front, Volume &volume, std::vector<unsigned char> &data, int &m, int &k, int &threadhold, float &gamma, int &cell_size){
    ImGui::Begin("Input Window");
    ImGui::Text("Camera Position: (%.2f, %.2f, %.2f)", camera_pos.x, camera_pos.y, camera_pos.z);
    ImGui::Text("Camera Front Position: (%.2f, %.2f, %.2f)", camera_front.x, camera_front.y, camera_front.z);
    std::vector<float> fd = volume.get_distribute();
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
        volume = Volume(data, volume.length, volume.width, volume.height);
        m = volume.get_distribute().size() - 1;
        volume.compute_gradient(1.0f, 255.0f);
        volume.compute_histogram2d(m, k);
    }

    ImGui::End();
}

/*
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
// */
void HistogramTFEditor(Volume &volume,
    std::vector<float> &tf_r,
    std::vector<float> &tf_g,
    std::vector<float> &tf_b,
    std::vector<float> &tf_alp,
    int cell_size){
    ImGui::Begin("Histogram & Transfer Function");

    // 1. Load 2D histogram
    auto hist2D = volume.get_histogram2d();  // M x K
    int M = (int) hist2D.size();
    int K = M > 0 ? (int) hist2D[0].size() : 0;
    int maxFreq = 0;
    for(int i = 0; i < M; ++i)
        for(int j = 0; j < K; ++j)
            maxFreq = std::max(maxFreq, hist2D[i][j]);

    // 2. Compute marginal 1D histograms and cumulative distributions
    std::vector<int> hist1D_x(K, 0), hist1D_y(M, 0);
    for(int i = 0; i < M; ++i){
        for(int j = 0; j < K; ++j){
            hist1D_x[j] += hist2D[i][j];
            hist1D_y[i] += hist2D[i][j];
        }
    }
    float sum_x = std::accumulate(hist1D_x.begin(), hist1D_x.end(), 0.0f);
    float sum_y = std::accumulate(hist1D_y.begin(), hist1D_y.end(), 0.0f);
    std::vector<float> cum_x(K, 0.0f), cum_y(M, 0.0f);
    float acc = 0.0f;
    for(int j = 0; j < K; ++j){
        acc += sum_x > 0 ? hist1D_x[j] / sum_x : 0.0f;
        cum_x[j] = acc;
    }
    acc = 0.0f;
    for(int i = 0; i < M; ++i){
        acc += sum_y > 0 ? hist1D_y[i] / sum_y : 0.0f;
        cum_y[i] = acc;
    }

    // 3. Create canvas
    ImVec2 canvasSize((float) K * cell_size, (float) M * cell_size);
    ImGui::InvisibleButton("canvas", canvasSize,
        ImGuiButtonFlags_MouseButtonLeft |
        ImGuiButtonFlags_MouseButtonRight);
    ImVec2 origin = ImGui::GetItemRectMin();
    ImVec2 canvasEnd = ImGui::GetItemRectMax();
    ImDrawList *dl = ImGui::GetWindowDrawList();

    // 4. Draw grayscale background
    for(int i = 0; i < M; ++i){
        for(int j = 0; j < K; ++j){
            float norm = maxFreq > 0 ? (float) hist2D[i][j] / maxFreq : 0.0f;
            int gray = 255 - (int) (norm * 255.0f);
            ImU32 col = IM_COL32(gray, gray, gray, 255);
            ImVec2 p0(origin.x + j * cell_size,
                origin.y + i * cell_size);
            ImVec2 p1(p0.x + cell_size, p0.y + cell_size);
            dl->AddRectFilled(p0, p1, col);
        }
    }


    // Draw CDF on horizontal axis (X marginal)
    if(K > 0){
        std::vector<ImVec2> polyX;
        polyX.reserve(K);
        for(int j = 0; j < K; ++j){
            float x = origin.x + j * cell_size + cell_size * 0.5f;
            float y = origin.y + canvasSize.y - cum_x[j] * canvasSize.y;
            polyX.emplace_back(x, y);
        }
        dl->AddPolyline(polyX.data(), (int) polyX.size(), IM_COL32(255, 255, 0, 200), false, 2.0f);
    }
    // Draw CDF on vertical axis (Y marginal)
    if(M > 0){
        std::vector<ImVec2> polyY;
        polyY.reserve(M);
        for(int i = 0; i < M; ++i){
            float x = origin.x + cum_y[i] * canvasSize.x;
            float y = origin.y + i * cell_size + cell_size * 0.5f;
            polyY.emplace_back(x, y);
        }
        dl->AddPolyline(polyY.data(), (int) polyY.size(), IM_COL32(0, 255, 255, 200), false, 2.0f);
    }
    if(K > 0){
        std::vector<ImVec2> polyX;
        polyX.reserve(K);
        for(int j = 0; j < K; ++j){
            float x = origin.x + j * cell_size + cell_size * 0.5f;
            float y = origin.y + canvasSize.y - cum_x[j] * canvasSize.y;
            polyX.emplace_back(x, y);
        }
        dl->AddPolyline(polyX.data(), (int) polyX.size(), IM_COL32(255, 255, 0, 200), false, 2.0f);
    }
    if(M > 0){
        std::vector<ImVec2> polyY;
        polyY.reserve(M);
        for(int i = 0; i < M; ++i){
            float x = origin.x + cum_y[i] * canvasSize.x;
            float y = origin.y + i * cell_size + cell_size * 0.5f;
            polyY.emplace_back(x, y);
        }
        dl->AddPolyline(polyY.data(), (int) polyY.size(), IM_COL32(0, 255, 255, 200), false, 2.0f);
    }

    // 6. Prepare TF editing on same canvas
    auto toCanvasTF = [&](int intensity, float value){
        float x = origin.x + (intensity / 255.0f) * canvasSize.x;
        float y = origin.y + canvasSize.y - value * canvasSize.y;
        return ImVec2(x, y);
    };
    struct ChannelPoint{ int intensity; float value; };
    static std::vector<ChannelPoint> redPts, greenPts, bluePts, alphaPts;
    struct Ch{ std::vector<ChannelPoint> *pts; ImU32 col; };
    Ch channels[4] = {
        { &redPts, IM_COL32(255,0,0,255) },
        { &greenPts, IM_COL32(0,255,0,255) },
        { &bluePts, IM_COL32(0,0,255,255) },
        { &alphaPts, IM_COL32(150,150,150,255) }
    };
    // Draw TF polylines and points
    for(int c = 0; c < 4; ++c){
        auto &pts = *channels[c].pts;
        std::sort(pts.begin(), pts.end(), [](auto &a, auto &b){return a.intensity < b.intensity; });
        if(pts.size() > 1){
            std::vector<ImVec2> poly;
            poly.reserve(pts.size());
            for(auto &p : pts) poly.push_back(toCanvasTF(p.intensity, p.value));
            dl->AddPolyline(poly.data(), (int) poly.size(), channels[c].col, false, 2.0f);
        }
        for(auto &p : pts){
            ImVec2 pc = toCanvasTF(p.intensity, p.value);
            dl->AddCircleFilled(pc, 4.0f, channels[c].col);
        }
    }
    // Interaction logic (drag, delete, add)... (same as before)
    static int currentChan = 0, activeChan = -1, activeIdx = -1;
    static bool dragging = false;
    bool hovered = ImGui::IsItemHovered();
    ImVec2 mpos = ImGui::GetIO().MousePos;
    int nearChan = -1, nearIdx = -1;
    float minDist = FLT_MAX;
    if(hovered){
        const float rad = 10.0f;
        for(int c = 0; c < 4; ++c){
            auto &pts = *channels[c].pts;
            for(int i = 0; i < (int) pts.size(); ++i){
                ImVec2 pc = toCanvasTF(pts[i].intensity, pts[i].value);
                float dx = mpos.x - pc.x, dy = mpos.y - pc.y;
                float d = sqrtf(dx * dx + dy * dy);
                if(d < rad && d < minDist){ minDist = d; nearChan = c; nearIdx = i; }
            }
        }
    }
    if(hovered && ImGui::IsMouseReleased(ImGuiMouseButton_Right)){
        if(nearChan >= 0 && nearIdx >= 0){
            auto &pts = *channels[nearChan].pts;
            if(nearIdx > 0 && nearIdx + 1 < (int) pts.size()) pts.erase(pts.begin() + nearIdx);
        }
    }
    if(hovered && ImGui::IsMouseDown(ImGuiMouseButton_Left)){
        if(!dragging){
            if(nearChan >= 0 && nearIdx >= 0){ dragging = true; activeChan = nearChan; activeIdx = nearIdx; }
            else{
                auto &pts = *channels[currentChan].pts;
                float ni = (mpos.x - origin.x) / canvasSize.x * 255.0f;
                float nv = (canvasEnd.y - mpos.y) / canvasSize.y;
                ni = std::clamp(ni, 0.0f, 255.0f);
                nv = std::clamp(nv, 0.0f, 1.0f);
                pts.push_back({ (int) ni,nv });
            }
        }
        else{
            auto &pts = *channels[activeChan].pts;
            if(activeIdx >= 0 && activeIdx < (int) pts.size()){
                float ni = (mpos.x - origin.x) / canvasSize.x * 255.0f;
                float nv = (canvasEnd.y - mpos.y) / canvasSize.y;
                ni = std::clamp(ni, 0.0f, 255.0f);
                nv = std::clamp(nv, 0.0f, 1.0f);
                if(activeIdx == 0) ni = 0.0f;
                if(activeIdx == (int) pts.size() - 1) ni = 255.0f;
                pts[activeIdx].intensity = (int) ni;
                pts[activeIdx].value = nv;
            }
        }
    }
    else{ dragging = false; activeChan = activeIdx = -1; }
    // Channel selection
    ImGui::RadioButton("Red", &currentChan, 0); ImGui::SameLine();
    ImGui::RadioButton("Green", &currentChan, 1); ImGui::SameLine();
    ImGui::RadioButton("Blue", &currentChan, 2); ImGui::SameLine();
    ImGui::RadioButton("Alpha", &currentChan, 3);

    // 7. Real-time interpolation to tf vectors
    auto process = [&](std::vector<ChannelPoint> &pts)->std::vector<float>{
        std::sort(pts.begin(), pts.end(), [](auto &a, auto &b){ return a.intensity < b.intensity; });
        if(pts.empty() || pts.front().intensity != 0) pts.insert(pts.begin(), { 0,pts.empty() ? 0.0f : pts.front().value });
        if(pts.back().intensity != 255) pts.push_back({ 255,pts.back().value });
        std::vector<float> tf(256);
        int seg = 0;
        for(int i = 0; i < 256; ++i){
            while(seg + 1 < (int) pts.size() && i > pts[seg + 1].intensity) seg++;
            int x0 = pts[seg].intensity, x1 = pts[seg + 1].intensity;
            float y0 = pts[seg].value, y1 = pts[seg + 1].value;
            float t = x1 == x0 ? 0.0f : (i - x0) / float(x1 - x0);
            tf[i] = (1.0f - t) * y0 + t * y1;
        }
        return tf;
    };
    tf_r = process(redPts);
    tf_g = process(greenPts);
    tf_b = process(bluePts);
    tf_alp = process(alphaPts);

    ImGui::End();
}
