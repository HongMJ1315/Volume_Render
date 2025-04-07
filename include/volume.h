#ifndef VOLUME_H
#define VOLUME_H

#include "glm/glm.hpp"
#include <iostream>
#include <vector>
#include <array>



class Volume{
    typedef unsigned char byte;
private:
    std::vector<std::vector<std::vector<float> > > data;
    std::vector<std::vector<std::vector<float>>> gradient_magnitude;
    std::vector<float> distribute;
    std::vector<std::vector<int>> histogram2d;
    int length, width, height;
public:
    Volume();
    Volume(std::vector<unsigned char> data, int length, int width, int height);
    ~Volume();
    void compute_gradient(float g_min, float g_max);
    std::vector<float> get_distribute();
    void compute_histogram2d(int M, int K);
    std::vector<std::vector<int>> get_histogram2d();
};


#endif