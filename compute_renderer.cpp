#include "compute_renderer.h"

#include <iostream>

void gl3::compute_renderer::create_output_texture()
{
    // Delete the existing texture if it exists
    if (output_texture != 0)
    {
        glDeleteTextures(1, &output_texture);
    }

    // Create texture for compute shader output
    glGenTextures(1, &output_texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, output_texture);

    // Initialize with empty data
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, window_size.x, window_size.y, 0, GL_RGBA, GL_FLOAT, nullptr);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    // Unbind texture
    glBindTexture(GL_TEXTURE_2D, 0);
}

void gl3::compute_renderer::create_display_quad()
{
    // Vertex data for a quad spanning the entire screen
    constexpr std::array quad_vertices = {
        // positions | texture coords
        -1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, -1.0f, 1.0f, 1.0f, 0.0f
    };

    // Create the VAO and the VBO
    glGenVertexArrays(1, &quad_vao);
    glGenBuffers(1, &quad_vbo);

    glBindVertexArray(quad_vao);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
    glBufferData(GL_ARRAY_BUFFER, quad_vertices.size() * sizeof(GLfloat), quad_vertices.data(), GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(0);
    
    // Texture coordinate attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), std::bit_cast<GLvoid*>(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    // Unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

gl3::compute_renderer::compute_renderer(scene_data& scene, const int width, const int height): scene(scene), window_size(width, height)
{
    // Load the compute shader
    compute_shader = std::make_unique<shader_class>("shaders/raytracer.comp");  
    

    // Load the display shader
    display_shader = std::make_unique<shader_class>("shaders/display.vert", "shaders/display.frag");

    // Create the output texture
    create_output_texture();

    // Create the display quad
    create_display_quad();

    // Check the compute shader capabilities
    std::array<int, 3> work_group_count{};
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_group_count[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_group_count[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &work_group_count[2]); 
    std::cout << "Max work group count: "
        << work_group_count[0] << ", "
        << work_group_count[1] << ", "
        << work_group_count[2] << std::endl;

    std::array<int, 3> work_group_size{};
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &work_group_size[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &work_group_size[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &work_group_size[2]);
    std::cout << "Max work group size: "
        << work_group_size[0] << ", "
        << work_group_size[1] << ", "
        << work_group_size[2] << std::endl;

    int work_group_invocations;
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &work_group_invocations);
    std::cout << "Max work group invocations: " << work_group_invocations << std::endl;
}

gl3::compute_renderer::~compute_renderer()
{
    if (output_texture != 0)
    {
        glDeleteTextures(1, &output_texture);
    }
    if (quad_vao != 0)
    {
        glDeleteVertexArrays(1, &quad_vao);
    }
    if (quad_vbo != 0)
    {
        glDeleteBuffers(1, &quad_vbo);
    }
}

void gl3::compute_renderer::resize(const int width, const int height)
{
    window_size = {width, height};

    // Update the output texture dimensions
    create_output_texture();
}

void gl3::compute_renderer::render() const
{
    // Update the UBOs with current scene data
    scene.update_UBOs();

    // Bind the compute shader
    compute_shader->activate();

    // Bind the output texture
    glBindImageTexture(0, output_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    // Dispatch the compute shader
    constexpr glm::ivec2 work_group_size = {16, 16};

    // Calculate the number of work groups to cover the entire texture
    const glm::ivec2 num_groups = (window_size + work_group_size - 1) / work_group_size;

    // Dispatch the compute shader
    glDispatchCompute(num_groups.x, num_groups.y, 1);

    // Wait for the compute shader to finish
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    // Unbind the compute shader
    shader_class::deactivate();
}

void gl3::compute_renderer::display() const
{
    // Clear screen
    glClear(GL_COLOR_BUFFER_BIT);

    // Activate the display shader
    display_shader->activate();

    // Bind the texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, output_texture);
    glUniform1i(glGetUniformLocation(display_shader->id, "rendered_texture"), 0);

    // Draw quad
    glBindVertexArray(quad_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    // Deactivate the shader
    shader_class::deactivate();
}
