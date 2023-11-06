#include <thread>
#include <vector>
#include "ray.h"
#include "color.h"
#include "camera.h"
#include "hittable_list.h"
#include "material.h"
#include <mutex>

std::mutex cerr_mutex;

color ray_colora(const ray& r, const color& background, const hittable& world, int depth) {
    hit_record rec;

    // If we've exceeded the ray bounce limit, no more light is gathered.
    if (depth <= 0)
        return color(0,0,0);

    // If the ray hits nothing, return the background color.
    if (!world.hit(r, 0.001, infinity, rec))
        return background;

    ray scattered;
    color attenuation;
    color emitted = rec.mat_ptr->emitted(rec.u, rec.v, rec.p);

    if (!rec.mat_ptr->scatter(r, rec, attenuation, scattered))
        return emitted;

    return emitted + attenuation * ray_colora(scattered, background, world, depth-1);
}

void render_tile(int tilenum, int x0, int y0, int x1, int y1, int max_depth, int width, int height, int samples_per_pixel, const camera& cam, const hittable& world, const color& background, unsigned char* &img, int &per) {
    for (int j = y0; j < y1; j++) {
        for (int i = x0; i < x1; i++) {
            color pixel_color(0, 0, 0);
            for (int s = 0; s < samples_per_pixel; ++s) {
                auto u = (i + random_double()) / (width-1);
                auto v = (j + random_double()) / (height-1);
                ray r = cam.get_ray(u, v);
                pixel_color += ray_colora(r, background, world, max_depth);
            }
            write_color(img, pixel_color, width, i, j, samples_per_pixel);
        }
    }
    per -= 1;
}

void flusher(int &per){
    while (per > 0){
        std::cerr << "\rTiles Left: " << per << ' ' << std::flush;
    }
}

void render_multithreaded(int max_depth, int width, int height, int samples_per_pixel, int num_threads, const camera& cam, const hittable& world, const color& background, unsigned char* &img) {
    std::vector<std::thread> threads;
    int tile_width = width / num_threads;
    int percentages = num_threads;
    for (int i = 0; i < num_threads; i++) {
        int x0 = i * tile_width;
        int x1 = (i+1) * tile_width;
        if (i == num_threads-1) {
            x1 = width;
        }
        threads.emplace_back(render_tile, i, x0, 0, x1, height, max_depth, width, height, samples_per_pixel, std::ref(cam), std::ref(world), std::ref(background), std::ref(img), std::ref(percentages));
    }
    threads.emplace_back(flusher, std::ref(percentages));
    for (auto& thread : threads) {
        thread.join();
    }
}