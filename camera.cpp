#include <GL/glew.h>
#include "camera.h"
#include "shader_class.h"
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

gl3::camera::camera(const int width, const int height, const glm::vec3 position): position(position), width(width), height(height)
{
    last_update_time = glfwGetTime();
}

void gl3::camera::matrix(const float fov_deg, const float nearPlane, const float farPlane, const shader_class& shader, const char* uniform) const
{
    const glm::mat4 view = glm::lookAt(position, position + orientation, up);
    const glm::mat4 projection = glm::perspective(glm::radians(fov_deg), static_cast<float>(width) / static_cast<float>(height), nearPlane, farPlane);

    glUniformMatrix4fv(glGetUniformLocation(shader.id, uniform), 1, GL_FALSE, glm::value_ptr(projection * view));
}

void gl3::camera::inputs(GLFWwindow* window)
{
    const double current_time = glfwGetTime();
    const auto delta_time = static_cast<float>(current_time - last_update_time);
    last_update_time = current_time;

    const float fixed_speed = speed * delta_time * 60.0f;
    
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    {
        position += fixed_speed * orientation;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    {
        position += fixed_speed * -glm::normalize(glm::cross(orientation, up));
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    {
        position += fixed_speed * -orientation;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    {
        position += fixed_speed * glm::normalize(glm::cross(orientation, up));
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
    {
        position += fixed_speed * up;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
    {
        position += fixed_speed * -up;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
    {
        speed = 0.4f;
    }
    else if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_RELEASE)
    {
        speed = 0.1f;
    }

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);  // DISABLED is better than HIDDEN

        if (first_click)
        {
            glfwSetCursorPos(window, static_cast<float>(width) / 2.0f, static_cast<float>(height) / 2);
            first_click = false;
        }

        double mouse_x;
        double mouse_y;
        glfwGetCursorPos(window, &mouse_x, &mouse_y);

        // Calculate rotation based on mouse movement from center
        const float rotation_x = sensitivity /** delta_time * 60.0f*/ * static_cast<float>(mouse_y - static_cast<float>(height) / 2.0f) / static_cast<float>(height);
        const float rotation_y = sensitivity /** delta_time * 60.0f*/ * static_cast<float>(mouse_x - (static_cast<float>(width) / 2)) / static_cast<float>(width);

        // Calculate new orientation with pitch (X) rotation
        const glm::vec3 right = glm::normalize(glm::cross(orientation, up));

        // Prevent camera from flipping by checking the angle with up vector
        // Allow more range (10-15 degrees) for natural movement
        if (const glm::vec3 new_orientation = glm::rotate(orientation, glm::radians(-rotation_x), right); !(glm::angle(new_orientation, up) <= glm::radians(10.0f) ||
              glm::angle(new_orientation, -up) <= glm::radians(10.0f)))
        {
            orientation = new_orientation;
        }

        // Apply yaw (Y) rotation around the Up axis
        orientation = glm::normalize(glm::rotate(orientation, glm::radians(-rotation_y), up));

        // Reset cursor position to center
        glfwSetCursorPos(window, static_cast<float>(width) / 2, static_cast<float>(height) / 2);
    }
    else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        first_click = true;
    }
}

void gl3::camera::move_key(GLFWwindow*, const int key, int, const int action, int)
{
    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        switch (key)
        {
        case GLFW_KEY_W:
            {
                position += speed * orientation;
                break;
            }
        case GLFW_KEY_A:
            {
                position += speed * -glm::normalize(glm::cross(orientation, up));
                break;
            }
        case GLFW_KEY_S:
            {
                position += speed * -orientation;
                break;
            }
        case GLFW_KEY_D:
            {
                position += speed * glm::normalize(glm::cross(orientation, up));
                break;
            }
        case GLFW_KEY_SPACE:
            {
                position += speed * up;
                break;
            }
        case GLFW_KEY_LEFT_CONTROL:
            {
                position += speed * -up;
                break;
            }
        default: break;
        }
    }
    if (key == GLFW_KEY_LEFT_SHIFT)
    {
        if (action == GLFW_PRESS)
        {
            speed = 4.0f;
        }
        else if (action == GLFW_RELEASE)
        {
            speed = 1.0f;
        }
    }
}
