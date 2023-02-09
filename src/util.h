#pragma once
#include <cmath>
#include <iomanip>
#include <iostream>
#include <math.h>
#include <vector>
using namespace std;
# define M_PI           3.14159265358979323846  /* pi */
// Function to create Gaussian filter
#define sigma 2
#define sigmaPow2 sigma*sigma
#define K  1
namespace
{
    std::vector<float> filterCreation(const int size)
    {
        auto IX = [&](int i, int j)
        {
            return i + j * size;
        };
        std::vector<float> kernel;
        
        // initialising standard deviation to 1.0
        double r, s = 2.0 * sigmaPow2;
        int size2 = int(size / 2);

        // sum is for normalization
        double sum = 0.0;
        for (int i = 0; i < size; ++i)
        {
            kernel.push_back(0.0f);
        }
        // generating 5x5 kernel
        for (int i = 0; i < size; i++) {
            float x = i - (size - 1) / 2;
            kernel[i] = K * exp((pow(x, 2) / s) * (-1));
            sum += kernel[i];
        }

        // normalising the Kernel
        for (int i = 0; i < kernel.size(); i++)
            kernel[i] /= sum;
        return kernel;
    }
}