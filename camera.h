#ifndef GL2_CAMERA_H
#define GL2_CAMERA_H


#include <glm/vec3.hpp>
#include "shader_class.h"
#include "GLFW/glfw3.h"

namespace gl3
{
    class camera
    {
    public:
        glm::vec3 position;
        glm::vec3 orientation = glm::vec3(0.0f, 0.0f, -1.0f);
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
        double last_update_time;

        bool first_click = true;

        int width;
        int height;

        float speed = 100.0f;
        float sensitivity = 100.0f;

        camera(int width, int height, glm::vec3 position);

        void matrix(float fov_deg, float nearPlane, float farPlane, const gl3::shader_class& shader, const char* uniform) const;

        void inputs(GLFWwindow* window);

        void move_key(GLFWwindow* window, int key, int scancode, int action, int mods);
    };
}

#endif //GL2_CAMERA_H
