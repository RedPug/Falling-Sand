#include "stdint.h"
#include <random>
#include <vector>

#ifndef COLOR_H
#define COLOR_H

struct Color {
    uint8_t r, g, b;

    Color(uint8_t red, uint8_t green, uint8_t blue) : r(red), g(green), b(blue) {}

    Color() : r(0), g(0), b(0) {}  // Default constructor initializes to black

    static Color fromSelection(const Color* colors, int size = 4, const int* weights = nullptr) {
        if (size <= 0) {
            return Color(0, 0, 0);  // Return black if no colors are provided
        }
        
        static std::random_device rd;
        static std::mt19937 gen(rd());

        std::vector<int> weight_vec;

        if (weights) {
            weight_vec.assign(weights, weights + size);
        } else {
            weight_vec.assign(size, 1);
        }
        std::discrete_distribution<int> color_dist(weight_vec.begin(), weight_vec.end());
        int idx = color_dist(gen);

        return colors[idx];
    }
};

#endif // COLOR_H