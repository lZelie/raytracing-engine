#ifndef BVH_H
#define BVH_H
#include "scene_data.h"
#include "glm/vec3.hpp"

// Maximum depth for BVH construction
constexpr auto MAX_BVH_DEPTH = 25;

// Structure to hold object reference during bvh construction
struct object_ref
{
    int index; // Index of the object
    int type; // Type of the object (0 = sphere, 1 = plane, 2 = triangle, 3=CSG)
    glm::vec3 centroid; // Centroid of the object
    glm::vec3 aabb_min; // AABB min of the object
    glm::vec3 aabb_max; // AABB max of the object
};

// BVH builder class
class bvh_builder
{
public:
    // Builds the BVH from scene objects
    static std::vector<scene_data::bvh_node> build_bvh(
        const std::array<scene_data::sphere_data, MAX_SPHERES>& spheres, int num_spheres,
        const std::array<scene_data::plane_data, MAX_PLANES>& planes, int num_planes,
        const std::array<scene_data::triangle_data, MAX_TRIANGLES>& triangles, int num_triangles,
        const std::array<scene_data::csg_sphere_data, MAX_CSG_SPHERES>& csg_spheres);

private:
    // Recursive BVH building function
    static int build_bvh_recursive(
        std::vector<scene_data::bvh_node>& nodes,
        std::vector<object_ref>& objects,
        int start, int end,
        int depth);

    // Computes the bounding box for a range of objects
    static void compute_bounds(
        const std::vector<object_ref>& objects,
        int start, int end,
        glm::vec3& out_min,
        glm::vec3& out_max);

    // Calculate the surface area of a bounding box
    static float calculate_surface_area(const glm::vec3& min, const glm::vec3& max);

    // Optimize BVH for cache locality
    static void optimize_bvh_for_cache(std::vector<scene_data::bvh_node>& nodes);
};


#endif //BVH_H
