#ifndef COLOR_H
#define COLOR_H

#include "vec3.h"
#include "main.h"

#include <iostream>

void write_color(unsigned char* &image_data, color pixel_color, int image_width, int i, int j, int samples_per_pixel) {
    auto r = pixel_color.x();
    auto g = pixel_color.y();
    auto b = pixel_color.z();

    // Divide the color by the number of samples.
    auto scale = 1.0 / samples_per_pixel;
    r = sqrt(scale * r);
    g = sqrt(scale * g);
    b = sqrt(scale * b);
    
    int index = 3 * (j * image_width + i);
    image_data[index] = static_cast<unsigned char>(256 * clamp(r, 0.0, 0.999));
    image_data[index + 1] = static_cast<unsigned char>(256 * clamp(g, 0.0, 0.999));
    image_data[index + 2] = static_cast<unsigned char>(256 * clamp(b, 0.0, 0.999));
}

#endif