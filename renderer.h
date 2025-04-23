#ifndef RENDERER_H
#define RENDERER_H
#include <memory>

#include "camera.h"
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
    private:
        GLFWwindow* window;
        float cameraFov;
        gl3::camera camera;

        // Scene data manager
        scene_data scene_data;
    
        // ImGui state
        bool show_ui;
        bool camera_mode;

        // Rendering state
        std::unique_ptr<gl3::shader_class> shader_program;
        std::unique_ptr<gl3::vao> quad_VAO;
        std::unique_ptr<gl3::vbo> quad_VBO;
        unsigned frame_acc = 0;
        double prev_fps_update = 0;
        double current_FPS = 0;
        void update_fps();
    
        // Initialize GLFW and create window
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
