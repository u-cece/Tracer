#pragma once

#include <random>

namespace tracer
{

class RNG
{
public:
    RNG() = default;
    float Uniform(float lower = 0.0f, float upper = 1.0f)
    {
        std::uniform_real_distribution distr(lower, upper);
        return distr(gen);
    }
    int Uniform(int lower, int upper)
    {
        std::uniform_int_distribution distr(lower, upper);
        return distr(gen);
    }
private:
    std::mt19937 gen;
};

}