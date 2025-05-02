#include "volume.h"
#include "reader.h"
#include <cmath>
#include <cfloat>
#include <algorithm>
#include <map>
#include <numeric>

Volume::Volume(){
    this->length = 0;
    this->width = 0;
    this->height = 0;
}
#define DENSITY 256

Volume::Volume(std::vector<byte> &data_in, int length, int width, int height){
    this->length = length;
    this->width = width;
    this->height = height;

    std::vector<int> hist(DENSITY, 0);
    for(auto v : data_in){
        if(v > 0) ++hist[v];
    }

    int totalCount = std::accumulate(hist.begin() + 1, hist.end(), 0);

    std::vector<float> cdf(DENSITY, 0.0f);
    if(totalCount > 0){
        float cum = 0.0f;
        for(int i = 1; i < DENSITY; ++i){
            cum += hist[i];
            cdf[i] = cum / totalCount;
        }
    }

    float cdf_min = 1.0f;
    for(int i = 1; i < DENSITY; ++i){
        if(hist[i] > 0 && cdf[i] < cdf_min){
            cdf_min = cdf[i];
        }
    }

    std::vector<byte> mapped(data_in.size(), 0);
    for(size_t idx = 0; idx < data_in.size(); ++idx){
        byte v = data_in[idx];
        if(v == 0){
            mapped[idx] = 0;
        }
        else{
            float p = (cdf[v] - cdf_min) / (1.0f - cdf_min);
            p = std::clamp(p, 0.0f, 1.0f);
            mapped[idx] = static_cast<byte>(std::round(p * (DENSITY - 2) + 1));
        }
    }

    data.assign(length,
        std::vector<std::vector<float>>(width,
        std::vector<float>(height, 0)));
    for(int x = 0; x < length; ++x){
        for(int y = 0; y < width; ++y){
            for(int z = 0; z < height; ++z){
                int index = x * width * height + y * height + z;
                data[x][y][z] = mapped[index];
            }
        }
    }

    distribute.assign(DENSITY, 0);
    for(auto mv : mapped){
        if(mv == 0) continue;
        ++distribute[mv];
    }
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
    histogram2d.assign(K, std::vector<int>(M, 0));

    float minVal = 1e30f, maxVal = -1e30f;
    float minGrad = 1e30f, maxGrad = -1e30f;

    for(int x = 1; x < length - 1; x++){
        for(int y = 1; y < width - 1; y++){
            for(int z = 1; z < height - 1; z++){
                float val = data[x][y][z];
                float grad = gradient_magnitude[x][y][z];
                if(val == 0.0f) continue;     

                minVal = std::min(minVal, val);
                maxVal = std::max(maxVal, val);
                minGrad = std::min(minGrad, grad);
                maxGrad = std::max(maxGrad, grad);
            }
        }
    }

    float rangeVal = (maxVal > minVal) ? maxVal - minVal : 1.0f;
    float rangeGrad = (maxGrad > minGrad) ? maxGrad - minGrad : 1.0f;


    for(int x = 1; x < length - 1; x++){
        for(int y = 1; y < width - 1; y++){
            for(int z = 1; z < height - 1; z++){
                float val = data[x][y][z];
                float grad = gradient_magnitude[x][y][z];
                if(val == 0.0f) continue;
                
                int bin_val = std::min(int((val - minVal) / rangeVal * M), M - 1);
                float norm_g = (grad - minGrad) / rangeGrad;
                int bin_grad = std::min(int(norm_g * K), K - 1);
                histogram2d[bin_grad][bin_val]++;
            }
        }
    }

    long long totalCount = 0;
    int       maxBinCnt = 0;
    for(int i = 0; i < K; i++){
        for(int j = 0; j < M; j++){
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
