#include "bvh.h"

#include <iostream>
#include <numeric>
#include <queue>
#include <unordered_map>

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

// Calculate surface area of a bounding box
float bvh_builder::calculate_surface_area(const glm::vec3& min, const glm::vec3& max)
{
    const glm::vec3 extent = max - min;
    return 2.0f * (extent.x * extent.y + extent.y * extent.z + extent.z * extent.x);
}

std::vector<scene_data::bvh_node> bvh_builder::build_bvh(
    const std::array<scene_data::sphere_data, MAX_SPHERES>& spheres, const int num_spheres,
    const std::array<scene_data::plane_data, MAX_PLANES>& planes, const int num_planes,
    const std::array<scene_data::triangle_data, MAX_TRIANGLES>& triangles, const int num_triangles,
    const std::array<scene_data::csg_sphere_data, MAX_CSG_SPHERES>& csg_spheres)
{
    std::vector<object_ref> objects;
    objects.reserve(num_spheres + num_planes + num_triangles + MAX_CSG_SPHERES);

    std::cout << "Building BVH with:" << std::endl;
    std::cout << "- " << num_spheres << " spheres" << std::endl;
    std::cout << "- " << num_planes << " planes" << std::endl;
    std::cout << "- " << num_triangles << " triangles" << std::endl;
    std::cout << "- " << MAX_CSG_SPHERES << " CSG spheres" << std::endl;

    // Add spheres to the object list
    for (int i = 0; i < num_spheres; i++)
    {
        object_ref ref{i, 0}; // type 0 = sphere

        // Calculate AABB
        calculate_sphere_aabb(spheres[i], ref.aabb_min, ref.aabb_max);
        ref.centroid = (ref.aabb_min + ref.aabb_max) * 0.5f;

        objects.push_back(ref);
    }

    // Add planes to the object list
    for (int i = 0; i < num_planes; i++)
    {
        object_ref ref{i, 1}; // type 1 = plane

        // Calculate AABB
        calculate_plane_aabb(planes[i], ref.aabb_min, ref.aabb_max);
        ref.centroid = (ref.aabb_min + ref.aabb_max) * 0.5f;

        objects.push_back(ref);
    }

    // Add triangles to the object list
    for (int i = 0; i < num_triangles; i++)
    {
        object_ref ref{i, 2}; // type 2 = triangle

        // Calculate AABB
        calculate_triangle_aabb(triangles[i], ref.aabb_min, ref.aabb_max);
        ref.centroid = (ref.aabb_min + ref.aabb_max) * 0.5f;

        objects.push_back(ref);
    }

    // Add CSG spheres to the object list
    for (int i = 0; i < MAX_CSG_SPHERES; i++)
    {
        object_ref ref{i, 3}; // type 3 = CSG sphere

        // Calculate AABB
        calculate_csg_sphere_aabb(csg_spheres[i], ref.aabb_min, ref.aabb_max);
        ref.centroid = (ref.aabb_min + ref.aabb_max) * 0.5f;

        objects.push_back(ref);
    }

    std::cout << "Total objects added to BVH: " << objects.size() << std::endl;

    // Initialize nodes vector
    std::vector<scene_data::bvh_node> nodes;
    nodes.reserve(MAX_BVH_NODES);

    // Start building the BVH recursively
    int root_index = build_bvh_recursive(nodes, objects, 0, static_cast<int>(objects.size()), 0);
    
    std::cout << "BVH built with " << nodes.size() << " nodes, root index: " << root_index << std::endl;
    
    if (nodes.empty()) {
        std::cerr << "ERROR: BVH construction failed - no nodes created!" << std::endl;
        // Create a dummy root node that includes all objects
        glm::vec3 aabb_min, aabb_max;
        compute_bounds(objects, 0, static_cast<int>(objects.size()), aabb_min, aabb_max);
        scene_data::bvh_node root(aabb_min, aabb_max, 0, static_cast<int>(objects.size()), 0);
        nodes.push_back(root);
    }

    return nodes;
}

int bvh_builder::build_bvh_recursive(
    std::vector<scene_data::bvh_node>& nodes,
    std::vector<object_ref>& objects,
    int start, int end,
    int depth)
{
    // Check for invalid range or max depth
    if (start >= end) {
        return -1;
    }
    
    // Compute the bounding box for all objects in this node
    glm::vec3 aabb_min, aabb_max;
    compute_bounds(objects, start, end, aabb_min, aabb_max);
    
    // If we've reached max depth or have a single object, create a leaf
    if (end - start == 1 || depth > MAX_BVH_DEPTH || nodes.size() >= MAX_BVH_NODES - 1) {
        // Just create one leaf node for all objects in the range
        const int node_index = static_cast<int>(nodes.size());
        scene_data::bvh_node leaf(aabb_min, aabb_max, objects[start].index, end - start, objects[start].type);
        leaf.split_axis = -1; // Mark as leaf
        nodes.push_back(leaf);
        
        std::cout << "Created leaf at depth " << depth << " with " << end - start 
                  << " objects of type " << objects[start].type << std::endl;
        
        return node_index;
    }
    
    // Always create an internal node and split the range
    const int current_node_index = static_cast<int>(nodes.size());
    
    // Add a placeholder node that will be filled in later
    nodes.emplace_back();
    
    // Choose the longest axis to split on
    int axis = 0;
    glm::vec3 extent = aabb_max - aabb_min;
    if (extent.y > extent.x) axis = 1;
    if (extent.z > extent[axis]) axis = 2;
    
    // Sort objects along the chosen axis
    std::sort(objects.begin() + start, objects.begin() + end,
              [axis](const object_ref& a, const object_ref& b) {
                  return a.centroid[axis] < b.centroid[axis];
              });
    
    // Find the middle point
    int mid = start + (end - start) / 2;
    
    // Make sure we don't create an empty child
    if (mid == start) mid = start + 1;
    if (mid == end) mid = end - 1;
    
    std::cout << "Creating internal node at depth " << depth 
              << " with range [" << start << ", " << end << "] and split at " << mid 
              << " on axis " << axis << std::endl;
    
    // Recursively build the children
    int left_child = build_bvh_recursive(nodes, objects, start, mid, depth + 1);
    int right_child = build_bvh_recursive(nodes, objects, mid, end, depth + 1);
    
    // Fill in the internal node
    nodes[current_node_index] = scene_data::bvh_node(aabb_min, aabb_max, left_child, right_child);
    nodes[current_node_index].split_axis = axis;
    
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

void bvh_builder::optimize_bvh_for_cache(std::vector<scene_data::bvh_node>& nodes)
{
    if (nodes.empty()) return;
    
    std::vector<scene_data::bvh_node> optimized_nodes;
    optimized_nodes.reserve(nodes.size());
    
    // Map of old node indices to new node indices
    std::unordered_map<int, int> index_map;
    
    // Traverse the BVH in breadth-first order
    std::queue<int> queue;
    queue.push(0);  // Root node
    
    while (!queue.empty())
    {
        int old_idx = queue.front();
        queue.pop();
        
        if (old_idx < 0) continue; // Skip invalid nodes
        
        // Map old index to new index
        int new_idx = static_cast<int>(optimized_nodes.size());
        index_map[old_idx] = new_idx;
        
        // Add node to optimized list
        optimized_nodes.push_back(nodes[old_idx]);
        
        // Add children to queue if this is an internal node
        if (nodes[old_idx].left_child >= 0)
        {
            queue.push(nodes[old_idx].left_child);
            queue.push(nodes[old_idx].right_child);
        }
    }
    
    // Update child indices to refer to new locations
    for (auto& node : optimized_nodes)
    {
        if (node.left_child >= 0)
        {
            node.left_child = index_map[node.left_child];
            node.right_child = index_map[node.right_child];
        }
    }
    
    // Replace original nodes with optimized ordering
    nodes = std::move(optimized_nodes);
}