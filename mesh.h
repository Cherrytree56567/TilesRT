#include "hittable.h"
#include "hittable_list.h"
#include "material.h"
#include "vec3.h"
#include <fstream>
#include <sstream>
#include <vector>

vec3 divvy(const vec3& v1, const vec3& v2) {
    return vec3(v1.x() / v2.x(), v1.y() / v2.y(), v1.z() / v2.z());
}

class triangle : public hittable {
public:
    triangle() {}
    triangle(vec3 v0, vec3 v1, vec3 v2, shared_ptr<material> mat)
        : v0(v0), v1(v1), v2(v2), mat_ptr(mat) {
        normal = unit_vector(cross(v1 - v0, v2 - v0));
    }

    virtual bool hit(const ray& r, double t_min, double t_max, hit_record& rec) const override {
        // Transform the ray into object space
        vec3 origin = divvy((r.origin() - position), scale);
        vec3 direction = rotate(divvy(r.direction(), scale), rotation);

        // Compute the intersection of the ray with the plane of the triangle
        double t = dot(v0 - origin, normal) / dot(direction, normal);
        if (t < t_min || t > t_max) {
            return false;
        }
        // Compute the intersection point
        vec3 p = origin + t * direction;
        // Check if the intersection point is inside the triangle
        double u = dot(cross(v2 - v0, p - v0), normal);
        double v = dot(cross(v0 - v1, p - v1), normal);
        if (u < 0 || v < 0 || u + v > 1) {
            return false;
        }
        // Fill in the hit record
        rec.t = t;
        rec.p = p * scale + position;
        rec.normal = rotate(normal, rotation);
        rec.mat_ptr = mat_ptr;
        return true;
    }

    virtual bool bounding_box(double time0, double time1, aabb& output_box) const override {
        // Compute the bounding box of the untransformed triangle
        vec3 min(fmin(fmin(v0.x(), v1.x()), v2.x()),
            fmin(fmin(v0.y(), v1.y()), v2.y()),
            fmin(fmin(v0.z(), v1.z()), v2.z()));
        vec3 max(fmax(fmax(v0.x(), v1.x()), v2.x()),
            fmax(fmax(v0.y(), v1.y()), v2.y()),
            fmax(fmax(v0.z(), v1.z()), v2.z()));
        // Transform the bounding box into object space
        min = rotate(divvy(min, scale), -rotation) + position;
        max = rotate(divvy(max, scale), -rotation) + position;
        // Fill in the output box
        output_box = aabb(min, max);
        return true;
    }

public:
    vec3 v0, v1, v2;
    vec3 normal;
    shared_ptr<material> mat_ptr;
    vec3 position; // new member variable for position
    vec3 rotation; // new member variable for rotation
    vec3 scale;    // new member variable for scale
};

triangle scale(const triangle& tri, const vec3& scal) {
    triangle scaled_tri = tri;
    scaled_tri.v0 = scaled_tri.v0 * scal;
    scaled_tri.v1 = scaled_tri.v1 * scal;
    scaled_tri.v2 = scaled_tri.v2 * scal;
    return scaled_tri;
}

std::vector<triangle> load_obj_file(const char* filename) {
    std::vector<triangle> triangles;
    std::vector<vec3> vertices;
    
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Error: Unable to open file " << filename << std::endl;
        return triangles;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;  // Skip empty or comment lines
        }

        std::istringstream iss(line);
        std::string type;
        iss >> type;
        if (type == "v") {
            float x, y, z;
            iss >> x >> y >> z;
            vertices.emplace_back(x, y, z);
        } else if (type == "f") {
            std::string v0, v1, v2;
            iss >> v0 >> v1 >> v2;
            int idx0 = std::stoi(v0) - 1;
            int idx1 = std::stoi(v1) - 1;
            int idx2 = std::stoi(v2) - 1;
            
            if (idx0 < 0 || idx0 >= vertices.size() ||
                idx1 < 0 || idx1 >= vertices.size() ||
                idx2 < 0 || idx2 >= vertices.size()) {
                std::cerr << "Error: Invalid vertex index in file " << filename << std::endl;
                return triangles;
            }
            
            triangle trib(vertices[idx0], vertices[idx1], vertices[idx2], make_shared<lambertian>(color(.65, .05, .05)));
            triangles.push_back(trib);
        }
    }
    
    return triangles;
}

class triangle_mesh : public hittable {
public:
    triangle_mesh(const char* filename, shared_ptr<material> mat, vec3 pos, vec3 rot, vec3 scal)
        : mat_ptr(mat) {
        std::vector<triangle> triangles_list = load_obj_file(filename);
        triangles = std::make_shared<hittable_list>();
        for (auto& tri : triangles_list) {
            tri.position = pos;
            tri.rotation = rot;
            tri.scale = scal;

            // Apply scaling to each vertex of the triangle
            tri.v0 = tri.v0 * scal;
            tri.v1 = tri.v1 * scal;
            tri.v2 = tri.v2 * scal;

            // Apply rotation to each vertex of the triangle
            tri.v0 = rotate(tri.v0, rot) + pos;
            tri.v1 = rotate(tri.v1, rot) + pos;
            tri.v2 = rotate(tri.v2, rot) + pos;

            // Create a new transformed triangle
            auto transformed_tri = std::make_shared<triangle>(
                tri.v0,
                tri.v1,
                tri.v2,
                mat_ptr);

            triangles->add(transformed_tri);
        }
    }

    virtual bool hit(const ray& r, double t_min, double t_max, hit_record& rec) const override {
        return triangles->hit(r, t_min, t_max, rec);
    }

    virtual bool bounding_box(double time0, double time1, aabb& output_box) const override {
        return triangles->bounding_box(time0, time1, output_box);
    }

public:
    shared_ptr<hittable_list> triangles;
    shared_ptr<material> mat_ptr;
};
