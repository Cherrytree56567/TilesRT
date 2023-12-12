#define STB_IMAGE_WRITE_IMPLEMENTATION
#define _CRT_SECURE_NO_WARNINGS true
#include <iostream>
#include <cmath>
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <stb/stb_image_write.h>

struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;
    Ray(const glm::vec3& origin, const glm::vec3& direction)
        : origin(origin), direction(direction) {}
};

class Object {
public:
    Object() {}
    virtual bool intersect(const Ray& ray, float& t) const = 0;
    virtual glm::vec3 getColor() const = 0;
    virtual glm::vec3 getNormal(const glm::vec3& hitPoint) const = 0;
    virtual ~Object() = default;
};

class Sphere : public Object {
public:
    glm::vec3 center;
    float radius;
    glm::vec3 color;

    Sphere(const glm::vec3& center, float radius, const glm::vec3& color)
        : center(center), radius(radius), color(color) {}

    bool intersect(const Ray& ray, float& t) const override {
        glm::vec3 oc = ray.origin - center;
        float a = glm::dot(ray.direction, ray.direction);
        float b = 2.0f * glm::dot(oc, ray.direction);
        float c = glm::dot(oc, oc) - radius * radius;
        float discriminant = b * b - 4 * a * c;

        if (discriminant > 0) {
            // Ray hits the sphere
            float t1 = (-b - sqrt(discriminant)) / (2.0f * a);
            float t2 = (-b + sqrt(discriminant)) / (2.0f * a);
            t = (t1 < t2) ? t1 : t2;
            return true;
        }

        return false;
    }

    glm::vec3 getColor() const override {
        return color;
    }

    glm::vec3 getNormal(const glm::vec3& hitPoint) const override {
        return glm::normalize(hitPoint - center);
    }
};

class Plane : public Object {
public:
    glm::vec3 normal;
    float distance;
    glm::vec3 color;

    Plane(const glm::vec3& normal, float distance, const glm::vec3& color)
        : normal(glm::normalize(normal)), distance(distance), color(color) {}

    bool intersect(const Ray& ray, float& t) const override {
        float denom = glm::dot(normal, ray.direction);

        if (std::fabs(denom) > 1e-6) {
            glm::vec3 toPlane = normal * distance - ray.origin;
            t = glm::dot(toPlane, normal) / denom;

            if (t >= 0) {
                return true;
            }
        }

        return false;
    }

    glm::vec3 getColor() const override {
        return color;
    }

    glm::vec3 getNormal(const glm::vec3& hitPoint) const override {
        return normal;
    }
};

glm::vec3 trace(const Ray& ray, const std::vector<std::unique_ptr<Object>>& objects) {
    float tClosest = std::numeric_limits<float>::infinity();
    const Object* closestObject = nullptr;

    for (const auto& object : objects) {
        float t;
        if (object->intersect(ray, t) && t < tClosest) {
            tClosest = t;
            closestObject = object.get();
        }
    }

    if (closestObject) {
        glm::vec3 hitPoint = ray.origin + ray.direction * tClosest;
        glm::vec3 normal = closestObject->getNormal(hitPoint);
        return closestObject->getColor() * std::max(0.0f, glm::dot(normal, -ray.direction));
    }

    // Background color (in this case, black)
    return { 0.0f, 0.0f, 0.0f };
}

int main() {
    std::vector<std::unique_ptr<Object>> objects;
    objects.emplace_back(std::make_unique<Sphere>(glm::vec3{ 0.0f, 0.0f, -5.0f }, 1.0f, glm::vec3{ 1.0f, 0.0f, 0.0f }));
    objects.emplace_back(std::make_unique<Plane>(glm::vec3{ 0.0f, 1.0f, 0.0f }, -1.0f, glm::vec3{ 0.0f, 1.0f, 0.0f }));

    int width = 800;
    int height = 600;

    std::vector<std::vector<glm::vec3>> image(width, std::vector<glm::vec3>(height));

    for (int i = 0; i < width; ++i) {
        for (int j = 0; j < height; ++j) {
            float u = static_cast<float>(i) / width;
            float v = static_cast<float>(j) / height;

            // Create a ray from the camera
            Ray ray({ 0.0f, 0.0f, 0.0f }, glm::normalize(glm::vec3(u - 0.5f, v - 0.5f, -1.0f)));

            // Trace the ray and set the pixel color
            image[i][j] = trace(ray, objects);
        }
    }

    // Output the image data (in this example, print colors to console)
    std::vector<unsigned char> image_data;

    for (int j = height - 1; j >= 0; --j) {
        for (int i = 0; i < width; ++i) {
            glm::vec3 color = image[i][j];

            // Scale color values to the range [0, 255]
            unsigned char r = static_cast<unsigned char>(255.0f * color.r);
            unsigned char g = static_cast<unsigned char>(255.0f * color.g);
            unsigned char b = static_cast<unsigned char>(255.0f * color.b);

            // Append color data to image_data
            image_data.push_back(r);
            image_data.push_back(g);
            image_data.push_back(b);
            image_data.push_back(255);  // Alpha channel (fully opaque)
        }
    }

    // Save the image to a PNG file using stb_image_write
    if (!stbi_write_png("output.png", width, height, 4, image_data.data(), width * 4)) {
        std::cerr << "Error saving PNG file." << std::endl;
    }

    return 0;
}
