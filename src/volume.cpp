#include "volume.h"
#include "reader.h"
#include "MarchingCubesTables.hpp"
#include "MarchingTetrahedraTables.hpp"
#include <cmath>
#include <cfloat>
#include <algorithm>
#include <map>

Volume::Volume(){
    this->length = 0;
    this->width = 0;
    this->height = 0;
}
#define DENSITY 256

Volume::Volume(std::vector<byte> &data, int length, int width, int height, float gamma, int freq_threadhold){
    /*-----------
    刪除低於域值的值
    -------------*/
    std::vector<int> tmp_distribute(DENSITY, 0);
    for(auto i : data) tmp_distribute[i] += (i == 0 ? 0 : 1);
    for(auto i : tmp_distribute) std::cout << i << " ";
    std::cout << std::endl;
    std::vector<byte> new_data;
    std::map<int, int> mp;
    int new_max_density = 0;
    for(int i = 0, index = 0; i < DENSITY; i++){
        if(tmp_distribute[i] >= freq_threadhold){
            mp[i] = index;
            index++;
            new_max_density = std::max(new_max_density, index);
        }
        else mp[i] = -1;
    }
    //重新映射
    for(auto i : data){
        if(mp[i] >= 0) new_data.push_back(mp[i]);
        else new_data.push_back(0);
    }


    /*-----------
    Gamma校正
    -------------*/
    float maxVal = 0, minVal = 255;
    for(int i = 0; i < length *width *height; i++){
        maxVal = std::max(maxVal, (float) new_data[i]);
        minVal = std::min(minVal, (float) new_data[i]);
    }
    // 計算 range 與其倒數，後面用來做 Normalization
    float range = maxVal - minVal;
    float invRange = (range > 0.0f ? 1.0f / range : 0.0f);
    for(auto &i : new_data){
        float norm = (i - minVal) * invRange;
        float corr = std::pow(norm, gamma);
        i = (byte) ((corr * range + minVal));
    }

    /*-----------
    映射回連續資料
    -------------*/
    std::vector<byte> uni_vec = new_data;
    std::sort(uni_vec.begin(), uni_vec.end());
    auto it = std::unique(uni_vec.begin(), uni_vec.end());
    std::vector<byte> final_data;
    int final_max_distribute;
    for(auto i : new_data){
        byte index = byte(std::lower_bound(uni_vec.begin(), it, i) - uni_vec.begin());
        final_data.push_back(index);
        final_max_distribute = std::max(final_max_distribute, (int) index);
    }
    std::vector<int> final_distribute(final_max_distribute + 1, 0);
    for(int i = 0; i < length; i++){
        std::vector<std::vector<float>> temp;
        for(int j = 0; j < width; j++){
            std::vector<float> temp2;
            for(int k = 0; k < height; k++){
                int idx = i * width * height + j * height + k;
                temp2.push_back(final_data[idx]);
                if(final_data[idx] == 0) continue;
                final_distribute[final_data[idx]]++;
            }
            temp.push_back(temp2);
        }
        this->data.push_back(temp);
    }

    this->length = length;
    this->width = width;
    this->height = height;
    this->distribute = final_distribute;
    std::cout << "set" << std::endl;
}
Volume::~Volume(){
    data.clear();
    gradient_magnitude.clear();
}

void Volume::compute_gradient(float g_min, float g_max){
    gradient_magnitude.resize(length, std::vector<std::vector<float>>(width, std::vector<float>(height, 0.0f)));

    for(int x = 1; x < length - 1; x++){
        for(int y = 1; y < width - 1; y++){
            for(int z = 1; z < height - 1; z++){
                float dx = (data[x + 1][y][z] - data[x - 1][y][z]) / 2.0f;
                float dy = (data[x][y + 1][z] - data[x][y - 1][z]) / 2.0f;
                float dz = (data[x][y][z + 1] - data[x][y][z - 1]) / 2.0f;

                float g = std::sqrt(dx * dx + dy * dy + dz * dz);

                if(g < g_min)
                    g = 2;
                if(g > g_max)
                    g = g_max;

                g = 20.0f * std::log2(g);

                gradient_magnitude[x][y][z] = g;
            }
        }
    }
}

std::vector<int> Volume::get_distribute(){
    return distribute;
}

void Volume::compute_histogram2d(int M, int K){
    histogram2d.assign(M, std::vector<int>(K, 0));

    // 先算出非零值的 min/max
    float minVal = 1e30f, maxVal = -1e30f;
    float minGrad = 1e30f, maxGrad = -1e30f;

    for(int x = 1; x < length - 1; x++){
        for(int y = 1; y < width - 1; y++){
            for(int z = 1; z < height - 1; z++){
                float val = data[x][y][z];
                float grad = gradient_magnitude[x][y][z];
                if(val == 0.0f) continue;               // ← skip zero

                minVal = std::min(minVal, val);
                maxVal = std::max(maxVal, val);
                minGrad = std::min(minGrad, grad);
                maxGrad = std::max(maxGrad, grad);
            }
        }
    }

    // 防止除以零
    float rangeVal = (maxVal > minVal) ? maxVal - minVal : 1.0f;
    float rangeGrad = (maxGrad > minGrad) ? maxGrad - minGrad : 1.0f;


    // 真正累計 histogram2d，同樣跳過 val==0
    for(int x = 1; x < length - 1; x++){
        for(int y = 1; y < width - 1; y++){
            for(int z = 1; z < height - 1; z++){
                float val = data[x][y][z];
                float grad = gradient_magnitude[x][y][z];
                if(val == 0.0f) continue;               // ← skip zero

                int bin_val = std::min(int((val - minVal) / rangeVal * M), M - 1);
                float norm_g = (grad - minGrad) / rangeGrad;
                int bin_grad = std::min(int(norm_g * K), K - 1);
                histogram2d[bin_val][bin_grad]++;
            }
        }
    }

    // (可選) debug 打印
    long long totalCount = 0;
    int       maxBinCnt = 0;
    for(int i = 0; i < M; i++){
        for(int j = 0; j < K; j++){
            totalCount += histogram2d[i][j];
            maxBinCnt = std::max(maxBinCnt, histogram2d[i][j]);
        }
    }
    std::cout << "totalCount: " << totalCount
        << ", maxBinCount: " << maxBinCnt << std::endl;
}

std::vector<std::vector<int>> Volume::get_histogram2d(){
    return histogram2d;
}
