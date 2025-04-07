#include "ui.h"



// 此函式會在同一個 Canvas 上同時繪製三條線 (R/G/B)，並支援拖曳、增刪折點
void line_editor_winodw(){
    // 建立一個視窗
    ImGui::Begin("RGB Transfer Function");

    // 讓使用者選擇「當前要新增折點的通道」
    static int currentChannel = 0; // 0=red, 1=green, 2=blue
    ImGui::RadioButton("Red", &currentChannel, 0); ImGui::SameLine();
    ImGui::RadioButton("Green", &currentChannel, 1); ImGui::SameLine();
    ImGui::RadioButton("Blue", &currentChannel, 2);

    // 設定畫布大小
    ImVec2 canvasSize(400, 250);
    // 建立 InvisibleButton 作為畫布
    ImGui::InvisibleButton("canvas", canvasSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
    ImVec2 canvasPos = ImGui::GetItemRectMin(); // 画布左上角
    ImVec2 canvasEnd = ImGui::GetItemRectMax(); // 画布右下角
    ImDrawList *drawList = ImGui::GetWindowDrawList();

    // 畫背景與網格
    drawList->AddRectFilled(canvasPos, canvasEnd, IM_COL32(50, 50, 50, 255));
    const float gridStepX = canvasSize.x / 10.0f;
    const float gridStepY = canvasSize.y / 10.0f;
    for(float x = canvasPos.x; x <= canvasEnd.x; x += gridStepX)
        drawList->AddLine(ImVec2(x, canvasPos.y), ImVec2(x, canvasEnd.y), IM_COL32(100, 100, 100, 40));
    for(float y = canvasPos.y; y <= canvasEnd.y; y += gridStepY)
        drawList->AddLine(ImVec2(canvasPos.x, y), ImVec2(canvasEnd.x, y), IM_COL32(100, 100, 100, 40));

    // 幫助函式：將 (intensity, value) 轉換到畫布座標
    auto toCanvas = [&](float intensity, float val){
        float x = canvasPos.x + (intensity / 255.0f) * canvasSize.x;
        float y = canvasPos.y + canvasSize.y - (val * canvasSize.y);
        return ImVec2(x, y);
    };

    // 我們把三條線分別處理，但都畫在同一張圖上
    // 先定義一個小結構方便我們存放
    struct ChannelData{
        std::vector<ChannelPoint> *pts;
        ImU32 color;
    };
    // 三個通道的資料
    ChannelData channels[3] = {
        { &redPoints,   IM_COL32(255, 0, 0, 255) },
        { &greenPoints, IM_COL32(0, 255, 0, 255) },
        { &bluePoints,  IM_COL32(0, 0, 255, 255) }
    };

    // 排序 + 建立 polyline
    auto buildPolyline = [&](ChannelData &ch){
        auto &vec = *(ch.pts);
        // 依 intensity 由小到大排序
        std::sort(vec.begin(), vec.end(), [](auto &a, auto &b){
            return a.intensity < b.intensity;
        });
        // 轉成 ImVec2
        std::vector<ImVec2> linePts;
        linePts.reserve(vec.size());
        for(auto &p : vec){
            linePts.push_back(toCanvas(p.intensity, p.value));
        }
        return linePts;
    };

    // 繪製三條線
    for(int c = 0; c < 3; c++){
        auto linePts = buildPolyline(channels[c]);
        if(linePts.size() > 1){
            drawList->AddPolyline(linePts.data(), (int) linePts.size(), channels[c].color, false, 2.0f);
        }
    }

    // --- 處理互動 ---
    // 1) 找到滑鼠最近的折點（距離小於某閾值）
    // 2) 拖曳、刪除、或新增折點

    // 靜態變數用來追蹤當前拖曳的點
    static int activeChannel = -1; // -1 表示目前沒有拖曳任何點
    static int activeIndex = -1;
    static bool dragging = false;

    bool canvasHovered = ImGui::IsItemHovered();
    ImVec2 mousePos = ImGui::GetIO().MousePos;
    float minDist = 1e6f;
    int nearestChannel = -1;
    int nearestIndex = -1;

    // 先掃描所有通道、所有點，找距離最近的折點
    if(canvasHovered){
        const float hitThreshold = 10.0f; // 感應半徑
        for(int c = 0; c < 3; c++){
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

    // --- 右鍵刪除 ---
    if(canvasHovered && ImGui::IsMouseReleased(ImGuiMouseButton_Right)){
        if(nearestChannel != -1 && nearestIndex != -1){
            auto &pts = *(channels[nearestChannel].pts);
            // 避免刪除首尾
            if(nearestIndex != 0 && nearestIndex != (int) pts.size() - 1){
                pts.erase(pts.begin() + nearestIndex);
            }
        }
    }

    // --- 左鍵拖曳 ---
    if(canvasHovered && ImGui::IsMouseDown(ImGuiMouseButton_Left)){
        if(!dragging){
            // 尚未拖曳中，就檢查是否點到某折點
            if(nearestChannel != -1 && nearestIndex != -1){
                activeChannel = nearestChannel;
                activeIndex = nearestIndex;
                dragging = true;
            }
        }
        else{
            // 拖曳中 => 更新該折點
            auto &pts = *(channels[activeChannel].pts);
            if(activeIndex >= 0 && activeIndex < (int) pts.size()){
                float newIntensity = (mousePos.x - canvasPos.x) / canvasSize.x * 255.0f;
                float newValue = (canvasEnd.y - mousePos.y) / canvasSize.y;
                newIntensity = std::clamp(newIntensity, 0.0f, 255.0f);
                newValue = std::clamp(newValue, 0.0f, 1.0f);
                // 避免首尾被拖到範圍外（若有需要也可不限制）
                if(activeIndex == 0){
                    newIntensity = 0.0f;
                }
                else if(activeIndex == (int) pts.size() - 1){
                    newIntensity = 255.0f;
                }
                pts[activeIndex].intensity = newIntensity;
                pts[activeIndex].value = newValue;
            }
        }
    }
    else{
        // 左鍵鬆開，結束拖曳
        dragging = false;
        activeChannel = -1;
        activeIndex = -1;
    }

    // --- 左鍵點擊空白區域 => 新增折點到「currentChannel」 ---
    // 為了避免跟「拖曳」衝突，我們在 MouseClicked 判斷
    if(canvasHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)){
        // 如果沒點到任何既有折點 => 視為新增
        if(nearestChannel == -1 || nearestIndex == -1){
            // 新增點到使用者選擇的通道
            auto &pts = *(channels[currentChannel].pts);
            float newIntensity = (mousePos.x - canvasPos.x) / canvasSize.x * 255.0f;
            float newValue = (canvasEnd.y - mousePos.y) / canvasSize.y;
            newIntensity = std::clamp(newIntensity, 0.0f, 255.0f);
            newValue = std::clamp(newValue, 0.0f, 1.0f);
            pts.push_back({ newIntensity, newValue });
        }
    }

    // 最後，畫出所有折點（圓形）
    const float pointRadius = 4.0f;
    for(int c = 0; c < 3; c++){
        auto &pts = *(channels[c].pts);
        ImU32 col = channels[c].color;
        for(int i = 0; i < (int) pts.size(); i++){
            ImVec2 pCanvas = toCanvas(pts[i].intensity, pts[i].value);
            drawList->AddCircleFilled(pCanvas, pointRadius, col);
            // 若想加外框，可再加 AddCircle()
        }
    }

    // 保留空間，讓 ImGui 知道這裡有一塊 400x250 的區域
    ImGui::Dummy(canvasSize);

    ImGui::End();
}

void input_window(glm::vec3 &camera_pos, glm::vec3 &camera_front, Volume &volume, int &m, int &k){
    ImGui::Begin("Input Window");
    ImGui::Text("Camera Position: (%.2f, %.2f, %.2f)", camera_pos.x, camera_pos.y, camera_pos.z);
    ImGui::Text("Camera Front Position: (%.2f, %.2f, %.2f)", camera_front.x, camera_front.y, camera_front.z);
    std::vector<float> d = volume.get_distribute();
    ImVec2 graph_size = ImVec2(0, 80);
    ImGui::PlotHistogram("My Histogram", d.data(), d.size(), 0, nullptr, FLT_MAX, FLT_MAX, graph_size);
    // textbox 讓使用者輸入 M, K
    ImGui::InputInt("M", &m);
    ImGui::InputInt("K", &k);
    if(ImGui::Button("Compute Histogram2D")){
        // 當按下按鈕時，計算 histogram2D
        volume.compute_histogram2d(m, k);
    }
    ImGui::End();
}


void histogram_window(Volume &volume){
    ImGui::Begin("Histogram2D Viewer");
    std::vector<std::vector<int>> histogram2D = volume.get_histogram2d();

    const int cell_size = 1; // 每個格子的寬高（你可以調整）
    const int M = histogram2D.size();
    const int K = histogram2D[0].size();

    // 尋找最大頻率
    int maxFreq = 0;
    for(int i = 0; i < M; ++i)
        for(int j = 0; j < K; ++j)
            maxFreq = std::max(maxFreq, histogram2D[i][j]);

    // 起始繪製位置
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


    // 保留空間讓 UI 知道這邊有畫東西
    ImGui::Dummy(ImVec2(K * cell_size, M * cell_size));

    ImGui::End();
}
