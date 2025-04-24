#ifndef COMPUTE_RENDERER_H
#define COMPUTE_RENDERER_H
#include <memory>

#include "scene_data.h"
#include "shader_class.h"


namespace gl3
{
    class compute_renderer
    {
        // Compute shader for ray tracing
        std::unique_ptr<shader_class> compute_shader;

        // Texture to store the rendered image
        GLuint output_texture{};

        // Quad for displaying the rendered image
        GLuint quad_vao{};
        GLuint quad_vbo{};

        // Shader for displaying the texture
        std::unique_ptr<shader_class> display_shader;

        // Scene data
        scene_data& scene;

        // Window properties
        glm::ivec2 window_size;

        // Create texture for a rendered output
        void create_output_texture();

        // Create quad for displaying the texture
        void create_display_quad();

    public:
        compute_renderer(scene_data& scene, int width, int height);
        ~compute_renderer();

        // Update texture dimensions if the window size changes
        void resize(int width, int height);

        // Render the scene
        void render() const;

        // Display the rendered image
        void display() const;
    };
}


#endif //COMPUTE_RENDERER_H
