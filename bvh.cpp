#include "bvh.h"

#include <iostream>
#include <numeric>

#include "glm/common.hpp"
#include "glm/geometric.hpp"

// Calculate the AABB for a sphere
void calculate_sphere_aabb(const scene_data::sphere_data& sphere, glm::vec3& out_min, glm::vec3& out_max)
{
    const float radius = sphere.radius;
    out_min = sphere.position - glm::vec3(radius);
    out_max = sphere.position + glm::vec3(radius);
}

// Calculate the AABB for a plane (approximated with a large box in the direction of the normal)
void calculate_plane_aabb(const scene_data::plane_data& plane, glm::vec3& out_min, glm::vec3& out_max)
{
    // Planes are technically infinite, but we'll create a large box in the normal direction
    constexpr float PLANE_EXTENT = 1000.0f;

    const glm::vec3 normal = glm::normalize(plane.normal);
    glm::vec3 tangent;

    // Find perpendicular vectors to normal
    if (std::abs(normal.x) < std::abs(normal.y))
    {
        tangent = glm::normalize(glm::cross(normal, glm::vec3(1.0f, 0.0f, 0.0f)));
    }
    else
    {
        tangent = glm::normalize(glm::cross(normal, glm::vec3(0.0f, 1.0f, 0.0f)));
    }
    const glm::vec3 bitangent = glm::normalize(glm::cross(normal, tangent));

    // Create a large thin box aligned with the plane
    const glm::vec3 corner1 = plane.position + tangent * PLANE_EXTENT + bitangent * PLANE_EXTENT - normal * 0.01f;
    const glm::vec3 corner2 = plane.position - tangent * PLANE_EXTENT - bitangent * PLANE_EXTENT + normal * 0.01f;

    out_min = glm::min(corner1, corner2);
    out_max = glm::max(corner1, corner2);
}

// Calculate the AABB for a triangle
void calculate_triangle_aabb(const scene_data::triangle_data& triangle, glm::vec3& out_min, glm::vec3& out_max)
{
    out_min = glm::min(glm::min(triangle.v1, triangle.v2), triangle.v3);
    out_max = glm::max(glm::max(triangle.v1, triangle.v2), triangle.v3);
}

// Calculate the AABB for a CSG sphere
void calculate_csg_sphere_aabb(const scene_data::csg_sphere_data& sphere, glm::vec3& out_min, glm::vec3& out_max)
{
    const float radius = sphere.radius;
    out_min = sphere.position - glm::vec3(radius);
    out_max = sphere.position + glm::vec3(radius);
}

std::vector<scene_data::bvh_node> bvh_builder::build_bvh(
    const std::array<scene_data::sphere_data, MAX_SPHERES>& spheres, const int num_spheres,
    const std::array<scene_data::plane_data, MAX_PLANES>& planes, const int num_planes,
    const std::array<scene_data::triangle_data, MAX_TRIANGLES>& triangles, const int num_triangles,
    const std::array<scene_data::csg_sphere_data, MAX_CSG_SPHERES>& csg_spheres)
{
    std::vector<object_ref> objects;
    objects.reserve(num_spheres + num_planes + num_triangles + MAX_CSG_SPHERES);

    // Add spheres to the object list
    for (int i = 0; i < num_spheres; i++)
    {
        object_ref ref{i, 0};

        // Calculate AABB
        calculate_sphere_aabb(spheres[i], ref.aabb_min, ref.aabb_max);
        ref.centroid = (ref.aabb_min + ref.aabb_max) * 0.5f;

        objects.push_back(ref);
    }

    // Add planes to the object list
    for (int i = 0; i < num_planes; i++)
    {
        object_ref ref{i, 1};

        // Calculate AABB
        calculate_plane_aabb(planes[i], ref.aabb_min, ref.aabb_max);
        ref.centroid = (ref.aabb_min + ref.aabb_max) * 0.5f;

        objects.push_back(ref);
    }

    // Add triangles to the object list
    for (int i = 0; i < num_triangles; i++)
    {
        object_ref ref{i, 2};

        // Calculate AABB
        calculate_triangle_aabb(triangles[i], ref.aabb_min, ref.aabb_max);
        ref.centroid = (ref.aabb_min + ref.aabb_max) * 0.5f;

        objects.push_back(ref);
    }

    // Add CSG spheres to the object list
    for (int i = 0; i < MAX_CSG_SPHERES; i++)
    {
        object_ref ref{i, 3};

        // Calculate AABB
        calculate_csg_sphere_aabb(csg_spheres[i], ref.aabb_min, ref.aabb_max);
        ref.centroid = (ref.aabb_min + ref.aabb_max) * 0.5f;

        objects.push_back(ref);
    }

    // Initialize nodes vector with a capacity that should be enough
    std::vector<scene_data::bvh_node> nodes;
    nodes.reserve(MAX_BVH_NODES);

    // Start building the BVH recursively
    build_bvh_recursive(nodes, objects, 0, static_cast<int>(objects.size()), 0);

    std::cout << "BVH built with " << nodes.size() << " nodes" << std::endl;

    return nodes;
}

int bvh_builder::build_bvh_recursive(
    std::vector<scene_data::bvh_node>& nodes,
    std::vector<object_ref>& objects,
    int start, int end,
    int depth)
{
    if (start >= end)
    {
        return -1;
    }

    // Check if we've reached the maximum number of nodes
    if (nodes.size() >= MAX_BVH_NODES)
    {
        std::cerr << "Warning: Maximum BVH nodes reached." << std::endl;
        // Create a leaf node with all remaining objects
        glm::vec3 aabb_min;
        glm::vec3 aabb_max;
        compute_bounds(objects, start, end, aabb_min, aabb_max);

        // Group by object type
        std::sort(objects.begin() + start, objects.begin() + end, [] (const object_ref& a, const object_ref& b) {return a.type < b.type; });

        // Create a leaf node for each object type
        int current_index = -1;

        for (int i = start; i < end; i++)
        {
            if (i == start || objects[i].type != objects[i - 1].type)
            {
                // New object type encountered
                if (i > start)
                {
                    // Finalize previous leaf node
                    scene_data::bvh_node leaf(aabb_min, aabb_max, objects[start].index, i - start, objects[start].type);
                    nodes.push_back(leaf);
                    current_index = static_cast<int>(nodes.size() - 1);
                }

                // Start a new group
                start = i;
            }
        }

        // Add the last group
        scene_data::bvh_node leaf(aabb_min, aabb_max, objects[start].index, end - start, objects[start].type);
        nodes.push_back(leaf);
        return static_cast<int>(nodes.size() - 1);
    }

    // Compute the bounding box for this node
    glm::vec3 aabb_min;
    glm::vec3 aabb_max;
    compute_bounds(objects, start, end, aabb_min, aabb_max);

    // If we have just one object, create a leaf node
    if (end - start == 1)
    {
        const object_ref& obj = objects[start];
        scene_data::bvh_node leaf(aabb_min, aabb_max, obj.index, 1, obj.type);
        nodes.push_back(leaf);
        return static_cast<int>(nodes.size() - 1);
    }

    // Pick the axis with the largest extent to split on
    glm::vec3 extent = aabb_max - aabb_min;
    int axis = 0;
    if (extent.y > extent.x) axis = 1;
    if (extent.z > extent[axis]) axis = 2;

    // If the extent is very small, create a leaf node
    if (extent[axis] < 0.0001f)
    {
        // Group by object type
        std::sort(objects.begin() + start, objects.begin() + end, [] (const object_ref& a, const object_ref& b) {return a.type < b.type; });

        // Create leaf nodes for each object type
        int first_leaf = -1;
        int last_added = -1;
        int current_start = start;

        for (int i = start; i < end; i++)
        {
            if (i == start || objects[i].type != objects[i - 1].type)
            {
                // End of a type group
                int count = i - current_start + 1;
                scene_data::bvh_node leaf(aabb_min, aabb_max, objects[current_start].index, count, objects[current_start].type);
                nodes.push_back(leaf);

                if (first_leaf == -1)
                {
                    first_leaf = static_cast<int>(nodes.size() - 1);
                }

                if (last_added != -1)
                {
                    // Link this leaf to the previous one
                    nodes[last_added].right_child = static_cast<int>(nodes.size() - 1);
                }

                last_added = static_cast<int>(nodes.size() - 1);
                current_start = i + 1;
            }
        }

        return first_leaf;
    }

    // Sort the objects along the selected axis
    int mid = std::midpoint(start, end);
    std::nth_element(
        objects.begin() + start,
        objects.begin() + mid,
        objects.begin() + end,
        [axis](const object_ref& a, const object_ref& b) {
            return a.centroid[axis] < b.centroid[axis];
        }
    );
    
    // Reserve space for this node
    auto current_node_index = static_cast<int>(nodes.size());
    nodes.emplace_back(); // Placeholder
    
    // Recursively build the left and right subtrees
    int left_child = build_bvh_recursive(nodes, objects, start, mid, depth + 1);
    int right_child = build_bvh_recursive(nodes, objects, mid, end, depth + 1);
    
    // Create the internal node
    nodes[current_node_index] = scene_data::bvh_node(aabb_min, aabb_max, left_child, right_child);
    
    return current_node_index;
}

void bvh_builder::compute_bounds(const std::vector<object_ref>& objects, const int start, const int end, glm::vec3& out_min,
    glm::vec3& out_max)
{
    out_min = glm::vec3(std::numeric_limits<float>::max());
    out_max = glm::vec3(std::numeric_limits<float>::lowest());

    for (int i = start; i < end; i++)
    {
        out_min = glm::min(out_min, objects[i].aabb_min);
        out_max = glm::max(out_max, objects[i].aabb_max);
    }
}
