#include "volume.h"
#include "reader.h"
#include "MarchingCubesTables.hpp"
#include "MarchingTetrahedraTables.hpp"
#include <cmath>
#include <cfloat>
#include <algorithm>

Volume::Volume(){
    this->length = 0;
    this->width = 0;
    this->height = 0;
}

Volume::Volume(std::vector<unsigned char> data, int length, int width, int height){
    distribute.resize(256, 0);

    float maxVal = 0, minVal = 255;
    for(int i = 0; i < length * width * height; i++){
        maxVal = std::max(maxVal, (float) data[i]);
        minVal = std::min(minVal, (float) data[i]);
        distribute[data[i]]++;
    }

    for(int i = 0; i < 256; i++){
        distribute[i] /= (maxVal - minVal);
    }

    for(int i = 0; i < length; i++){
        std::vector<std::vector<float>> temp;
        for(int j = 0; j < width; j++){
            std::vector<float> temp2;
            for(int k = 0; k < height; k++){
                temp2.push_back((float) data[i * width * height + j * height + k]);
            }
            temp.push_back(temp2);
        }
        this->data.push_back(temp);
    }
    this->length = length;
    this->width = width;
    this->height = height;
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

std::vector<float> Volume::get_distribute(){
    return distribute;
}

void Volume::compute_histogram2d(int M, int K){
    histogram2d.assign(M, std::vector<int>(K, 0));

    float minVal = 255.0f, maxVal = 0.0f;
    float minGrad = FLT_MAX, maxGrad = -FLT_MAX;

    for(int x = 1; x < length - 1; x++){
        for(int y = 1; y < width - 1; y++){
            for(int z = 1; z < height - 1; z++){
                float val = data[x][y][z];
                float grad = gradient_magnitude[x][y][z];

                minVal = std::min(minVal, val);
                maxVal = std::max(maxVal, val);

                minGrad = std::min(minGrad, grad);
                maxGrad = std::max(maxGrad, grad);
            }
        }
    }

    std::cout << "minVal: " << minVal << ", maxVal: " << maxVal << std::endl;
    std::cout << "minGrad: " << minGrad << ", maxGrad: " << maxGrad << std::endl;

    // 防止除以零：若範圍太小，設為 1 避免數值錯誤
    float rangeVal = maxVal - minVal;
    if(rangeVal == 0)
        rangeVal = 1.0f;
    float rangeGrad = maxGrad - minGrad;
    if(rangeGrad == 0)
        rangeGrad = 1.0f;

    float gamma = 1.f; // 可根據需要調整此參數

    for(int x = 1; x < length - 1; x++){
        for(int y = 1; y < width - 1; y++){
            for(int z = 1; z < height - 1; z++){
                float val = data[x][y][z];
                float grad = gradient_magnitude[x][y][z];

                int bin_val = std::min(int((val - minVal) / rangeVal * M), M - 1);
                float norm_grad = (grad - minGrad) / rangeGrad;
                norm_grad = std::pow(norm_grad, gamma);
                int bin_grad = std::min(int(norm_grad * K), K - 1);

                histogram2d[bin_val][bin_grad]++;
            }
        }
    }

    // Debug：計算 histogram2d 中的總計數與最大計數
    long long totalCount = 0;
    int maxBinCount = 0;
    for(int i = 0; i < M; i++){
        for(int j = 0; j < K; j++){
            totalCount += histogram2d[i][j];
            maxBinCount = std::max(maxBinCount, histogram2d[i][j]);
        }
    }
    std::cout << "totalCount: " << totalCount << std::endl;
    std::cout << "maxBinCount: " << maxBinCount << std::endl;
}

std::vector<std::vector<int>> Volume::get_histogram2d(){
    return histogram2d;
}
