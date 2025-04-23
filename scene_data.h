#ifndef SCENE_DATA_H
#define SCENE_DATA_H
#include "camera.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

// Maximum number of objects in the scene
constexpr int MAX_SPHERES = 256;
constexpr int MAX_PLANES = 128;
constexpr int MAX_TRIANGLES = 256;
constexpr int MAX_CSG_SPHERES = 4;
constexpr int MAX_MATERIALS = MAX_SPHERES + MAX_PLANES + MAX_TRIANGLES + MAX_CSG_SPHERES;

// UBO binding points
constexpr int CAMERA_UBO_BINDING = 0;
constexpr int OBJECTS_UBO_BINDING = 1;
constexpr int LIGHTING_UBO_BINDING = 2;

// SceneData class to manage all scene objects and UBOs
class scene_data
{
public:
    // Camera and view data
    struct camera_data
    {
        glm::vec2 window_size;
        std::array<float, 2> padding1;
        glm::vec3 position;
        float padding2;
        glm::vec3 target;
        float fov;
        float exposure_time;
        int time_samples;
        float focal_distance;
        float aperture_size;        
    };

    // Structure for scene objects
    struct sphere_data
    {
        glm::vec3 position;
        float radius;
        glm::vec3 velocity;
        float padding1{};

        sphere_data(const glm::vec3 position = glm::vec3(0.0f), const float radius = 0.0f, const glm::vec3 velocity = glm::vec3(0.0f)): position(position),
            radius(radius), velocity(velocity)
        {
        }
    };

    struct plane_data
    {
        glm::vec3 position;
        float padding1{};
        glm::vec3 normal;
        float padding2{};

        plane_data(const glm::vec3 position = glm::vec3(0.0f),
                   const glm::vec3 normal = glm::vec3(0.0f)): position(position), normal(normal)
        {
        }
    };

    struct triangle_data
    {
        glm::vec3 v1;
        float padding1{};
        glm::vec3 v2;
        float padding2{};
        glm::vec3 v3;
        float padding3{};

        triangle_data(const glm::vec3& v1 = glm::vec3(0.0f), const glm::vec3& v2 = glm::vec3(0.0f),
                      const glm::vec3& v3 = glm::vec3(0.0f)): v1(v1), v2(v2), v3(v3)
        {
        }
    };

    struct csg_sphere_data
    {
        glm::vec3 position;
        float radius;

        csg_sphere_data(const glm::vec3& position = glm::vec3(0.0f), const float radius = 0.0f): position(position),
            radius(radius)
        {
        }
    };

    struct scene_objects
    {
        std::array<sphere_data, MAX_SPHERES> spheres{};
        std::array<plane_data, MAX_PLANES> planes{};
        std::array<triangle_data, MAX_TRIANGLES> triangles{};
        std::array<csg_sphere_data, MAX_CSG_SPHERES> csg_spheres{};
        int num_spheres{};
        int num_planes{};
        int num_triangles{};
        int padding1{};

        struct material
        {
            glm::vec3 diffuse;
            float padding1{};
            glm::vec3 specular;
            float padding2{};
            glm::vec3 ambient;
            float shininess = 32.0f;
            float reflection_coefficient = 0.0f;
            float refraction_coefficient = 0.0f;
            float refraction_index = 1.0f;
            float glossiness = 0.0f;
            glm::vec3 absorption;
            float padding3{};

            material(const glm::vec3& diffuse = glm::vec3(1.0f),
                     const glm::vec3& specular = glm::vec3(1.0f),
                     const glm::vec3& ambient = glm::vec3(1.0f),
                     const float shininess = 32.0f,
                     const float reflection_coefficient = 0.0f,
                     const float refraction_coefficient = 0.0f,
                     const float refraction_index = 1.0f,
                     const float glossiness = 0.0f,
                     const glm::vec3& absorption = glm::vec3(0.0f))
                : diffuse(diffuse),
                  specular(specular),
                  ambient(ambient),
                  shininess(shininess),
                  reflection_coefficient(reflection_coefficient),
                  refraction_coefficient(refraction_coefficient),
                  refraction_index(refraction_index),
                  glossiness(glossiness),
                  absorption(absorption)
            {
            }
        };

        std::array<material, MAX_SPHERES> sphere_materials{};
        std::array<material, MAX_PLANES> plane_materials{};
        std::array<material, MAX_TRIANGLES> triangle_materials{};
        std::array<material, MAX_CSG_SPHERES> csg_sphere_materials{};
    };

    // Lighting data
    struct lighting_data
    {
        glm::vec4 light_position;
        glm::vec3 light_color;
        float padding1;
        glm::vec3 ambient_light;
        int light_type;
        int sample_rate;
        unsigned int recursion_depth;
        bool use_fresnel = false;
        float light_radius = 1.0f;
        int shadow_samples = 1;
    };

    scene_data();
    ~scene_data();

    // Initialize UBOs and default scene
    void initialize();

    // Update UBOs with current data
    void update_UBOs() const;

    // Accessors for scene data
    camera_data& get_camera() { return camera; }
    scene_objects& get_objects() { return objects; }
    lighting_data& get_lighting() { return lighting; }

    // Reset to default scene
    void reset_to_default();

    // Add/modify objects
    void add_sphere(const glm::vec3& position, float radius);
    void add_plane(const glm::vec3& position, const glm::vec3& normal);
    void add_triangle(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3);
    void update_csg_spheres(const std::array<csg_sphere_data, MAX_CSG_SPHERES>& csg_spheres);

private:
    camera_data camera{};
    scene_objects objects{};
    lighting_data lighting{};

    // UBO handles
    GLuint camera_UBO;
    GLuint objects_UBO;
    GLuint lighting_UBO;

    // Create the uniform buffer objects
    void create_UBOs();
};


#endif //SCENE_DATA_H
