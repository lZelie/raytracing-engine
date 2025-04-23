#include "scene_data.h"

#include <iostream>

#include "renderer.h"

scene_data::scene_data() : camera_UBO(0), objects_UBO(0), lighting_UBO(0)
{
    // Initialize default camera settings
    camera.window_size = {INITIAL_WIDTH, INITIAL_HEIGHT};
    camera.position = {0.0f, 1.0f, 1.0f};
    camera.target = {0.0f, 0.0f, 0.0f};
    camera.fov = CAMERA_FOV;

    // Initialize counters
    objects.num_spheres = 0;
    objects.num_planes = 0;
    objects.num_triangles = 0;

    // Initialize lighting
    lighting.light_position = {0.0f, 10.0f, 15.0f, 0.9f};
    lighting.light_color = {1.0f, 1.0f, 0.99f};
    lighting.ambient_light = {0.1f, 0.1f, 0.1f};
    lighting.light_type = 0;
    lighting.sample_rate = 1;
    lighting.recursion_depth = 0;
}

scene_data::~scene_data()
{
    // Delete UBOs
    if (camera_UBO != 0)
        glDeleteBuffers(1, &camera_UBO);
    if (objects_UBO != 0)
        glDeleteBuffers(1, &objects_UBO);
    if (lighting_UBO != 0)
        glDeleteBuffers(1, &lighting_UBO);
}

void scene_data::initialize()
{
    // Create UBOs
    create_UBOs();

    // Set up the default scene
    reset_to_default();

    // Update UBOs with initial data
    update_UBOs();
}

void scene_data::create_UBOs()
{
    // Create Camera UBO
    glGenBuffers(1, &camera_UBO);
    glBindBuffer(GL_UNIFORM_BUFFER, camera_UBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(camera_data), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, CAMERA_UBO_BINDING, camera_UBO);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // Create Objects UBO
    glGenBuffers(1, &objects_UBO);
    glBindBuffer(GL_UNIFORM_BUFFER, objects_UBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(scene_objects), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, OBJECTS_UBO_BINDING, objects_UBO);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // Create Lighting UBO
    glGenBuffers(1, &lighting_UBO);
    glBindBuffer(GL_UNIFORM_BUFFER, lighting_UBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(lighting_data), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, LIGHTING_UBO_BINDING, lighting_UBO);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void scene_data::update_UBOs() const
{
    // Update Camera UBO
    glBindBuffer(GL_UNIFORM_BUFFER, camera_UBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(camera_data), &camera);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // Update Objects UBO
    glBindBuffer(GL_UNIFORM_BUFFER, objects_UBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(scene_objects), &objects);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // Update Lighting UBO
    glBindBuffer(GL_UNIFORM_BUFFER, lighting_UBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(lighting_data), &lighting);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void scene_data::reset_to_default()
{
    // Reset camera
    camera.position = glm::vec3(0.0f, 1.0f, 1.0f);
    camera.target = glm::vec3(0.0f, 0.0f, 0.0f);
    camera.fov = CAMERA_FOV;
    camera.exposure_time = 1.0f;
    camera.time_samples = 1;

    camera.focal_distance = 15.0f;
    camera.aperture_size = 0.3f;  // A moderate DOF effect

    // Reset object counters
    objects.num_spheres = 5;
    objects.num_planes = 6;
    objects.num_triangles = 8;

    // Add default spheres from Renderer.h
    objects.spheres[0] = {{5.0f, -35.0f, -10.0f}, 1.0f};
    objects.spheres[1] = {{14.0f, -35.0f, -16.0f}, 1.0f};
    objects.spheres[2] = {{15.0f, -35.0f, 15.0f}, 1.0f};
    objects.spheres[3] = {{15.0f, -35.0f, 15.0f}, 0.99f};
    objects.spheres[4] = {{10.0f, -35.0f, -16.0f}, 1.0f};

    // Add default planes
    // Bottom face
    objects.planes[0] = {{0.0f, -40.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
    // Top face
    objects.planes[1] = {{0.0f, 40.0f, 0.0f}, {0.0f, -1.0f, 0.0f}};
    // Left face
    objects.planes[2] = {{-40.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
    // Right face
    objects.planes[3] = {{40.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}};
    // Back face
    objects.planes[4] = {{0.0f, 0.0f, -40.0f}, {0.0f, 0.0f, 1.0f}};
    // Front face
    objects.planes[5] = {{0.0f, 0.0f, 40.0f}, {0.0f, 0.0f, -1.0f}};

    // Add default triangles pyramid
    objects.triangles[0] = {{3.0f, -1.0f, 3.0f}, {3.0f, -1.0f, 5.0f}, {4.0f, 2.0f, 4.0f}};
    objects.triangles[1] = {{3.0f, -1.0f, 5.0f}, {5.0f, -1.0f, 5.0f}, {4.0f, 2.0f, 4.0f}};
    objects.triangles[2] = {{5.0f, -1.0f, 5.0f}, {5.0f, -1.0f, 3.0f}, {4.0f, 2.0f, 4.0f}};
    objects.triangles[3] = {{5.0f, -1.0f, 3.0f}, {3.0f, -1.0f, 3.0f}, {4.0f, 2.0f, 4.0f}};
    objects.triangles[4] = {{3.0f, -1.0f, 3.0f}, {4.0f, -3.0f, 4.0f}, {3.0f, -1.0f, 5.0f}};
    objects.triangles[5] = {{3.0f, -1.0f, 5.0f}, {4.0f, -3.0f, 4.0f}, {5.0f, -1.0f, 5.0f}};
    objects.triangles[6] = {{5.0f, -1.0f, 5.0f}, {4.0f, -3.0f, 4.0f}, {5.0f, -1.0f, 3.0f}};
    objects.triangles[7] = {{5.0f, -1.0f, 3.0f}, {4.0f, -3.0f, 4.0f}, {3.0f, -1.0f, 3.0f}};

    // Default CSG spheres
    objects.csg_spheres[0] = {{-1.0f, 2.0f, 0.0f}, 1.5f};
    objects.csg_spheres[1] = {{1.0f, 2.0f, 0.0f}, 1.5f};
    objects.csg_spheres[2] = {{0.0f, 2.7f, -0.3f}, 0.8f};
    objects.csg_spheres[3] = {{0.0f, 2.8f, 0.3f}, 0.8f};

    // Reset lighting to default
    lighting.light_position = glm::vec4(0.0f, 10.0f, 15.0f, 0.9f);
    lighting.light_color = glm::vec3(1.0f, 1.0f, 0.99f);
    lighting.ambient_light = glm::vec3(0.1f, 0.1f, 0.1f);
    lighting.light_type = 0;
    lighting.sample_rate = 1;
    lighting.recursion_depth = 0;

    // Reset materials to default
    objects.sphere_materials[0] = {{0.8f, 0.2f, 0.2f}, {1.0f, 1.0f, 1.0f}, {0.1f, 0.1f, 0.1f}, 32.0f, 1.0f};
    objects.sphere_materials[1] = {{0.0f, 0.0f, 0.0f}, {0.9f, 0.9f, 0.9f}, {0.1f, 0.1f, 0.1f}, 128.0f, 0.0f, 1.0f, 1.333f,0.0f, {0.8, 0.0, 0.0}};
    objects.sphere_materials[2] = {{0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.1f, 0.1f, 0.1f}, 256.0f, 0.0f, 1.0f, 1.5f};
    objects.sphere_materials[3] = {{0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.1f, 0.1f, 0.1f}, 256.0f, 0.0f, 1.0f, 1.0f};
    objects.sphere_materials[4] = {{0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.1f, 0.1f, 0.1f}, 256.0f, 0.5f, 0.5f, 1.12f};

    objects.plane_materials[0] = {};
    
    objects.triangle_materials[0] = {{0.0f, 0.0f, 0.0f}, {0.5f, 0.5f, 0.5f}, {0.1f, 0.1f, 0.1f}, 16.0f, 0.0f, 1.0f, 2.24f};
    objects.triangle_materials[1] = {{0.0f, 0.0f, 0.0f}, {0.5f, 0.5f, 0.5f}, {0.1f, 0.1f, 0.1f}, 16.0f, 0.0f, 1.0f, 2.24f};
    objects.triangle_materials[2] = {{0.0f, 0.0f, 0.0f}, {0.5f, 0.5f, 0.5f}, {0.1f, 0.1f, 0.1f}, 16.0f, 0.0f, 1.0f, 2.24f};
    objects.triangle_materials[3] = {{0.0f, 0.0f, 0.0f}, {0.5f, 0.5f, 0.5f}, {0.1f, 0.1f, 0.1f}, 16.0f, 0.0f, 1.0f, 2.24f};
    objects.triangle_materials[4] = {{0.0f, 0.0f, 0.0f}, {0.5f, 0.5f, 0.5f}, {0.1f, 0.1f, 0.1f}, 16.0f, 0.0f, 1.0f, 2.24f};
    objects.triangle_materials[5] = {{0.0f, 0.0f, 0.0f}, {0.5f, 0.5f, 0.5f}, {0.1f, 0.1f, 0.1f}, 16.0f, 0.0f, 1.0f, 2.24f};
    objects.triangle_materials[6] = {{0.0f, 0.0f, 0.0f}, {0.5f, 0.5f, 0.5f}, {0.1f, 0.1f, 0.1f}, 16.0f, 0.0f, 1.0f, 2.24f};
    objects.triangle_materials[7] = {{0.0f, 0.0f, 0.0f}, {0.5f, 0.5f, 0.5f}, {0.1f, 0.1f, 0.1f}, 16.0f, 0.0f, 1.0f, 2.24f};

    objects.csg_sphere_materials[0] = {{0.8f, 0.2f, 0.2f}, {1.0f, 1.0f, 1.0f}, {0.1f, 0.1f, 0.1f}, 32.0f};
    objects.csg_sphere_materials[1] = {{0.8f, 0.2f, 0.2f}, {1.0f, 1.0f, 1.0f}, {0.1f, 0.1f, 0.1f}, 32.0f};
    objects.csg_sphere_materials[2] = {{0.2f, 0.2f, 0.8f}, {1.0f, 1.0f, 1.0f}, {0.1f, 0.1f, 0.1f}, 32.0f};
    objects.csg_sphere_materials[3] = {{0.2f, 0.8f, 0.2f}, {1.0f, 1.0f, 1.0f}, {0.1f, 0.1f, 0.1f}, 32.0f};
    
}

void scene_data::add_sphere(const glm::vec3& position, const float radius)
{
    if (objects.num_spheres < MAX_SPHERES)
    {
        objects.spheres[objects.num_spheres] = {position, radius};
        objects.num_spheres++;
    }
    else
    {
        std::cerr << "Maximum number of spheres reached." << std::endl;
    }
}

void scene_data::add_plane(const glm::vec3& position, const glm::vec3& normal)
{
    if (objects.num_planes < MAX_PLANES)
    {
        objects.planes[objects.num_planes] = {position, normal};
        objects.num_planes++;
    }
    else
    {
        std::cerr << "Maximum number of planes reached." << std::endl;
    }
}

void scene_data::add_triangle(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3)
{
    if (objects.num_triangles < MAX_TRIANGLES)
    {
        objects.triangles[objects.num_triangles] = {v1, v2, v3};
        objects.num_triangles++;
    }
    else
    {
        std::cerr << "Maximum number of triangles reached." << std::endl;
    }
}

void scene_data::update_csg_spheres(const std::array<csg_sphere_data, MAX_CSG_SPHERES>& csg_spheres)
{
    for (int i = 0; i < MAX_CSG_SPHERES; i++)
    {
        objects.csg_spheres[i] = csg_spheres[i];
    }
}





