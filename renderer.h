#ifndef RENDERER_H
#define RENDERER_H
#include <memory>

#include "camera.h"
#include "compute_renderer.h"
#include "scene_data.h"
#include "vao.h"
#include "GLFW/glfw3.h"

// Application constants
constexpr int INITIAL_WIDTH = 640;
constexpr int INITIAL_HEIGHT = 480;
constexpr float CAMERA_FOV = 1.5f;
constexpr char WINDOW_TITLE[] = "RTX on";
constexpr int SAMPLE_RATE = 1;
constexpr int FPS_UPDATE_DELAY = 1;

namespace gl3
{
    class renderer {
        GLFWwindow* window = nullptr;
        float camera_fov = CAMERA_FOV;
        camera camera{INITIAL_WIDTH, INITIAL_HEIGHT, {0.0f, 1.0f, 1.0f}};

        // Scene data manager
        scene_data scene_data;
    
        // ImGui state
        bool show_ui = true;
        bool camera_mode = false;
        bool use_compute_shader = false; // Flag to toggle between compute and fragment shader

        // Rendering state
        std::unique_ptr<shader_class> shader_program;
        std::unique_ptr<vao> quad_VAO;
        std::unique_ptr<vbo> quad_VBO;

        // Compute shader approach
        std::unique_ptr<compute_renderer> compute_rend;
        
        unsigned frame_acc = 0;
        double prev_fps_update = 0;
        double current_FPS = 0;
        void update_fps();
    
        // Initialize GLFW and create the window
        void init_window();

        // Initialize ImGui
        void init_imgui();

        // Render ImGui UI
        void render_ui();

        // Set up callback functions
        void setup_callbacks();

        // Initialize quad for rendering
        void init_quad();

    public:
        renderer();
    
        ~renderer();

        void render_frame();

        [[nodiscard]] bool should_close() const;
    };
}




#endif //RENDERER_H
