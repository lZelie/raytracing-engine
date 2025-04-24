#include "renderer.h"

#include <iostream>
#include <sstream>
#include <format>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "glm/gtc/type_ptr.hpp"

void gl3::renderer::init_window()
{
    glfwInit();

    // Set OpenGL version and profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    // Create the window
    window = glfwCreateWindow(INITIAL_WIDTH, INITIAL_HEIGHT, WINDOW_TITLE, nullptr, nullptr);
    glfwMakeContextCurrent(window);

    // Set callbacks
    setup_callbacks();

    // Initialize GLEW
    if (const GLenum err = glewInit(); err != GLEW_OK)
    {
        std::cerr << "Error: " << glewGetErrorString(err) << std::endl;
        exit(EXIT_FAILURE);
    }
}

void gl3::renderer::init_imgui()
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 430");
}


void gl3::renderer::setup_callbacks()
{
    // Window resize callback
    glfwSetWindowSizeCallback(window, [](GLFWwindow* win, int width, int height)
    {
        const auto renderer = static_cast<gl3::renderer*>(glfwGetWindowUserPointer(win));
        renderer->scene_data.get_camera().window_size = {width, height};
        if (renderer->compute_rend){renderer->compute_rend->resize(width, height);}
    });

    // Keyboard input callback
    glfwSetKeyCallback(window, [](GLFWwindow* win, const int key, int, const int action, int)
    {
        const auto renderer = static_cast<gl3::renderer*>(glfwGetWindowUserPointer(win));

        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(win, GLFW_TRUE);
        }
        if (key == GLFW_KEY_TAB && action == GLFW_PRESS)
        {
            renderer->show_ui = !renderer->show_ui;
        }
        if (key == GLFW_KEY_C && action == GLFW_PRESS)
        {
            renderer->camera_mode = !renderer->camera_mode;
        }
        if (key == GLFW_KEY_1 && action == GLFW_PRESS)
        {
            renderer->scene_data.get_lighting().light_type =
                renderer->scene_data.get_lighting().light_type == 0 ? 1 : 0;
        }
        if (key == GLFW_KEY_R && action == GLFW_PRESS)
        {
            // Toggle between compute and fragment shader
            renderer->use_compute_shader = !renderer->use_compute_shader;
            std::cout << "Using " << (renderer->use_compute_shader ? "compute" : "fragment") << " shader" << std::endl;
        }
        if (key == GLFW_KEY_KP_ADD && action == GLFW_PRESS)
        {
            renderer->scene_data.get_lighting().recursion_depth++;
        }
        if (key == GLFW_KEY_KP_SUBTRACT && action == GLFW_PRESS && renderer->scene_data.get_lighting().recursion_depth >
            0)
        {
            renderer->scene_data.get_lighting().recursion_depth--;
        }
        if (key == GLFW_KEY_U && action == GLFW_PRESS)
        {
            renderer->shader_program = std::make_unique<shader_class>("shaders/default.vert", "shaders/default.frag");
            if (renderer->compute_rend)
            {
                int width;
                int height;
                glfwGetFramebufferSize(win, &width, &height);
                renderer->compute_rend.reset();
                renderer->compute_rend = std::make_unique<compute_renderer>(renderer->scene_data, width, height);
            }
        }
    });

    // Store this instance for callbacks
    glfwSetWindowUserPointer(window, this);
}

void gl3::renderer::init_quad()
{
    const std::vector vertices = {
        -1.0f, 1.0f, // Top-left
        -1.0f, -1.0f, // Bottom-left
        1.0f, 1.0f, // Top-right
        1.0f, -1.0f // Bottom-right
    };

    quad_VAO->bind();
    quad_VBO = std::make_unique<gl3::vbo>(vertices);
    quad_VAO->linkAttrib(*quad_VBO, 0, 2, GL_FLOAT, 0, nullptr);
}

void gl3::renderer::render_ui()
{
    // Start ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Main UI window
    ImGui::Begin("Raytracer Settings");

    // FPS display
    ImGui::Text("FPS: %.1f", current_FPS);
    ImGui::Separator();

    // Render method selection
    if (ImGui::Checkbox("Use Compute Shader", &use_compute_shader) && use_compute_shader && !compute_rend)
    {
        // If we're toggling to the compute shader, and it doesn't exist, create it
        int width;
        int height;
        glfwGetWindowSize(window, &width, &height);
        compute_rend = std::make_unique<compute_renderer>(scene_data, width, height);
    }
    ImGui::SameLine();
    ImGui::Text("Press R to toggle between compute and fragment shader");
    ImGui::Separator();

    // Tab selection
    if (ImGui::BeginTabBar("SettingsTabs"))
    {
        // Scene tab
        if (ImGui::BeginTabItem("Scene"))
        {
            // Light settings
            ImGui::Text("Light Settings");
            auto& lighting = scene_data.get_lighting();

            // Light position
            if (glm::vec3 light_pos = {lighting.light_position}; ImGui::DragFloat3(
                "Light Position", glm::value_ptr(light_pos), 0.1f))
            {
                lighting.light_position.x = light_pos.x;
                lighting.light_position.y = light_pos.y;
                lighting.light_position.z = light_pos.z;
            }

            // Light intensity
            if (float light_intensity = lighting.light_position.w; ImGui::SliderFloat(
                "Light Intensity", &light_intensity, 0.0f, 2.0f))
            {
                lighting.light_position.w = light_intensity;
            }

            // Light color
            ImGui::ColorEdit3("Light Color", glm::value_ptr(lighting.light_color));

            // Ambient light
            ImGui::ColorEdit3("Ambient Light", glm::value_ptr(lighting.ambient_light));

            // Light type (Phong or Blinn-Phong)
            constexpr std::array<const char*, 2> light_types = {"Phong", "Blinn-Phong"};
            if (int light_type = lighting.light_type; ImGui::Combo("Lighting Type", &light_type, light_types.data(),
                                                                   light_types.size()))
            {
                lighting.light_type = light_type;
            }

            // Sample rate for anti-aliasing
            if (int sample_rate = lighting.sample_rate; ImGui::SliderInt("Sample Rate", &sample_rate, 1, 4))
            {
                lighting.sample_rate = sample_rate;
            }

            // Max recursion depth
            if (auto recursion_depth = static_cast<int>(lighting.recursion_depth); ImGui::SliderInt(
                "Recursion Depth", &recursion_depth, 0, 8))
            {
                lighting.recursion_depth = recursion_depth;
            }

            // Fresnel
            if (auto use_fresnel = lighting.use_fresnel; ImGui::Checkbox("Use Fresnel", &use_fresnel))
            {
                lighting.use_fresnel = use_fresnel;
            }

            // Soft shadows
            ImGui::Text("Soft Shadows Settings");

            // Light radius
            if (float light_radius = lighting.light_radius; ImGui::SliderFloat(
                "Light Radius", &light_radius, 0.0f, 10.0f))
            {
                lighting.light_radius = light_radius;
            }

            // Shadow samples
            if (int shadow_samples = lighting.shadow_samples; ImGui::SliderInt(
                "Shadow Samples", &shadow_samples, 1, 256))
            {
                lighting.shadow_samples = shadow_samples;
            }

            ImGui::Separator();

            // CSG Objects
            ImGui::Text("CSG Objects");
            auto& objects = scene_data.get_objects();

            for (int i = 0; i < MAX_CSG_SPHERES; i++)
            {
                ImGui::PushID(i);

                if (std::string label = std::format("CSG Sphere {}", std::to_string(i + 1)); ImGui::TreeNode(
                    label.c_str()))
                {
                    glm::vec3 pos = objects.csg_spheres[i].position;
                    float radius = objects.csg_spheres[i].radius;

                    bool changed = false;
                    changed |= ImGui::DragFloat3("Position", glm::value_ptr(pos), 0.1f);
                    changed |= ImGui::DragFloat("Radius", &radius, 0.1f, 5.0f);

                    if (changed)
                    {
                        objects.csg_spheres[i] = {pos, radius};
                    }

                    ImGui::TreePop();
                }

                ImGui::PopID();
            }

            ImGui::EndTabItem();
        }

        // Camera Tab
        if (ImGui::BeginTabItem("Camera"))
        {
            auto& cam = scene_data.get_camera();

            // Camera position
            if (glm::vec3 camera_position = cam.position; ImGui::DragFloat3(
                "Position", glm::value_ptr(camera_position), 0.1f))
            {
                cam.position = camera_position;
            }

            // Camera target
            if (glm::vec3 camera_target = cam.target; ImGui::DragFloat3("Target", glm::value_ptr(camera_target), 0.1f))
            {
                cam.target = camera_target;
            }

            // Camera FOV
            if (float fov = cam.fov; ImGui::SliderFloat("FOV", &fov, 0.0f, glm::pi<float>()))
            {
                cam.fov = fov;
            }

            // Camera exposure time
            if (float exposure_time = cam.exposure_time; ImGui::DragFloat("Exposure Time", &exposure_time, 0.1f))
            {
                cam.exposure_time = exposure_time;
            }

            // Camera time samples
            if (int time_samples = cam.time_samples; ImGui::SliderInt("Time Samples", &time_samples, 1, 10))
            {
                cam.time_samples = time_samples;
            }

            // Depth of Field controls
            if (float focal_distance = cam.focal_distance; ImGui::SliderFloat(
                "Focal Distance", &focal_distance, 0.1f, 100.0f))
            {
                cam.focal_distance = focal_distance;
            }
            ImGui::Text("Distance at which objects appear in perfect focus");

            if (float aperture_size = cam.aperture_size; ImGui::SliderFloat(
                "Aperture Size", &aperture_size, 0.0f, 2.0f))
            {
                cam.aperture_size = aperture_size;
            }
            ImGui::Text("0 = Everything in focus, higher = more blur");

            // Add a "Focus on Closest" button
            if (ImGui::Button("Focus on Closest Object (Only for spheres)")) {
                // Calculate the distance to the closest object or scene center
                auto closest_pos = glm::vec3(FLT_MAX);
                for (int i = 0; i < scene_data.get_objects().num_spheres; i++)
                {
                    const auto sphere_pos = scene_data.get_objects().spheres[i].position;
                    if (glm::distance(sphere_pos, cam.position) < glm::distance(closest_pos, cam.position))
                    {
                        closest_pos = sphere_pos;
                    }
                }
                cam.focal_distance = glm::length(closest_pos - cam.position);
            }

            ImGui::Text("Controls: Use WASD to move camera when in camera mode");
            ImGui::Text("Press C to toggle camera mode");

            ImGui::EndTabItem();
        }

        // Objects tab
        if (ImGui::BeginTabItem("Objects"))
        {
            auto& objects = scene_data.get_objects();

            // Spheres
            if (ImGui::TreeNode("Spheres"))
            {
                // Add new sphere button
                if (ImGui::Button("Add Sphere") && objects.num_spheres < MAX_SPHERES)
                {
                    scene_data.add_sphere({0.0f, 0.0f, 0.0f}, 1.0f);
                }

                ImGui::Separator();

                // List all spheres with edit controls
                for (int i = 0; i < objects.num_spheres; i++)
                {
                    ImGui::PushID(i);

                    if (std::string label = std::format("Sphere {}", std::to_string(i + 1)); ImGui::TreeNode(
                        label.c_str()))
                    {
                        glm::vec3 pos = objects.spheres[i].position;
                        float radius = objects.spheres[i].radius;
                        glm::vec3 velocity = objects.spheres[i].velocity;

                        bool changed = false;
                        changed |= ImGui::DragFloat3("Position", glm::value_ptr(pos), 0.1f);
                        changed |= ImGui::DragFloat("Radius", &radius, 0.1f, 5.0f);
                        changed |= ImGui::DragFloat3("Velocity", glm::value_ptr(velocity), 0.1f);

                        if (changed)
                        {
                            objects.spheres[i] = {pos, radius, velocity};
                        }

                        ImGui::TreePop();
                    }

                    ImGui::PopID();
                }

                ImGui::TreePop();
            }

            // Planes
            if (ImGui::TreeNode("Planes"))
            {
                // Show first few planes (walls of the scene)
                for (int i = 0; i < std::min(objects.num_planes, 6); i++)
                {
                    ImGui::PushID(i);

                    if (std::string label = std::format("Plane {}", std::to_string(i + 1)); ImGui::TreeNode(
                        label.c_str()))
                    {
                        glm::vec3 pos = objects.planes[i].position;
                        glm::vec3 normal = objects.planes[i].normal;

                        bool changed = false;
                        changed |= ImGui::DragFloat3("Position", glm::value_ptr(pos), 0.1f);
                        changed |= ImGui::DragFloat3("Normal", glm::value_ptr(normal), 0.1f);

                        if (changed)
                        {
                            objects.planes[i].position = pos;
                            objects.planes[i].normal = glm::normalize(normal);
                        }

                        ImGui::TreePop();
                    }

                    ImGui::PopID();
                }

                ImGui::TreePop();
            }

            ImGui::EndTabItem();
        }

        // Materials tab
        if (ImGui::BeginTabItem("Materials"))
        {
            auto& objects = scene_data.get_objects();

            // Sphere materials
            if (ImGui::TreeNode("Sphere materials"))
            {
                // Show sphere materials
                for (int i = 0; i < std::min(objects.num_spheres, 6); i++)
                {
                    ImGui::PushID(i);
                    if (std::string label = std::format("Sphere material {}", std::to_string(i + 1)); ImGui::TreeNode(
                        label.c_str()))
                    {
                        auto& sphere_material = objects.sphere_materials[i];
                        bool changed = false;

                        changed |= ImGui::ColorEdit3("Diffuse", glm::value_ptr(sphere_material.diffuse));
                        changed |= ImGui::ColorEdit3("Specular", glm::value_ptr(sphere_material.specular));
                        changed |= ImGui::ColorEdit3("Ambient", glm::value_ptr(sphere_material.ambient));
                        changed |= ImGui::DragFloat("Shininess", &sphere_material.shininess, 1.0f, 0.0f);
                        changed |= ImGui::SliderFloat("Reflection coefficient", &sphere_material.reflection_coefficient,
                                                      0.0f, 1.0f);
                        changed |= ImGui::SliderFloat("Refraction coefficient", &sphere_material.refraction_coefficient,
                                                      0.0f, 1.0f);
                        changed |= ImGui::SliderFloat("Refraction index", &sphere_material.refraction_index, 0.0f,
                                                      10.0f);
                        changed |= ImGui::SliderFloat("Glossiness", &sphere_material.glossiness, 0.0f, 1.0f);
                        changed |= ImGui::ColorEdit3("Absorption", glm::value_ptr(sphere_material.absorption));

                        if (changed)
                        {
                            sphere_material.reflection_coefficient = glm::clamp(
                                sphere_material.reflection_coefficient, 0.0f, 1.0f);
                            sphere_material.refraction_coefficient = glm::clamp(
                                sphere_material.refraction_coefficient, 0.0f,
                                1.0f - sphere_material.reflection_coefficient);

                            objects.sphere_materials[i] = sphere_material;
                        }

                        ImGui::TreePop();
                    }

                    ImGui::PopID();
                }

                ImGui::TreePop();
            }
            // Planes materials
            if (ImGui::TreeNode("Planes materials (Ignored for checkerboard)"))
            {
                // Show sphere materials
                for (int i = 0; i < std::min(objects.num_planes, 6); i++)
                {
                    ImGui::PushID(i);
                    if (std::string label = std::format("Planes material {}", std::to_string(i + 1)); ImGui::TreeNode(
                        label.c_str()))
                    {
                        auto& plane_material = objects.plane_materials[i];
                        bool changed = false;

                        changed |= ImGui::ColorEdit3("Diffuse", glm::value_ptr(plane_material.diffuse));
                        changed |= ImGui::ColorEdit3("Specular", glm::value_ptr(plane_material.specular));
                        changed |= ImGui::ColorEdit3("Ambient", glm::value_ptr(plane_material.ambient));
                        changed |= ImGui::DragFloat("Shininess", &plane_material.shininess, 1.0f, 0.0f);
                        changed |= ImGui::SliderFloat("Reflection coefficient", &plane_material.reflection_coefficient,
                                                      0.0f, 1.0f);
                        changed |= ImGui::SliderFloat("Refraction coefficient", &plane_material.refraction_coefficient,
                                                      0.0f, 1.0f);
                        changed |= ImGui::SliderFloat("Refraction index", &plane_material.refraction_index,
                                                      0.0f, 10.0f);
                        changed |= ImGui::SliderFloat("Glossiness", &plane_material.glossiness, 0.0f, 1.0f);

                        changed |= ImGui::ColorEdit3("Absorption", glm::value_ptr(plane_material.absorption));

                        if (changed)
                        {
                            plane_material.reflection_coefficient = glm::clamp(
                                plane_material.reflection_coefficient, 0.0f,
                                1.0f - objects.plane_materials[i].refraction_coefficient);
                            plane_material.refraction_coefficient = glm::clamp(
                                plane_material.refraction_coefficient, 0.0f,
                                1.0f - objects.plane_materials[i].reflection_coefficient);

                            objects.plane_materials[i] = plane_material;
                        }

                        ImGui::TreePop();
                    }

                    ImGui::PopID();
                }

                ImGui::TreePop();
            }
            // Triangle materials
            if (ImGui::TreeNode("Triangle materials"))
            {
                // Show sphere materials
                for (int i = 0; i < std::min(objects.num_triangles, 6); i++)
                {
                    ImGui::PushID(i);
                    if (std::string label = std::format("Triangle material {}", std::to_string(i + 1)); ImGui::TreeNode(
                        label.c_str()))
                    {
                        auto& triangle_material = objects.triangle_materials[i];
                        bool changed = false;

                        changed |= ImGui::ColorEdit3("Diffuse", glm::value_ptr(triangle_material.diffuse));
                        changed |= ImGui::ColorEdit3("Specular", glm::value_ptr(triangle_material.specular));
                        changed |= ImGui::ColorEdit3("Ambient", glm::value_ptr(triangle_material.ambient));
                        changed |= ImGui::DragFloat("Shininess", &triangle_material.shininess, 1.0f, 0.0f);
                        changed |= ImGui::SliderFloat("Reflection coefficient",
                                                      &triangle_material.reflection_coefficient,
                                                      0.0f, 1.0f);
                        changed |= ImGui::SliderFloat("Refraction coefficient",
                                                      &triangle_material.refraction_coefficient,
                                                      0.0f, 1.0f);
                        changed |= ImGui::SliderFloat("Refraction index", &triangle_material.refraction_index, 0.0f,
                                                      10.0f);
                        changed |= ImGui::SliderFloat("Glossiness", &triangle_material.glossiness, 0.0f, 1.0f);
                        changed |= ImGui::ColorEdit3("Absorption", glm::value_ptr(triangle_material.absorption));

                        if (changed)
                        {
                            triangle_material.reflection_coefficient = glm::clamp(
                                triangle_material.reflection_coefficient, 0.0f,
                                1.0f - objects.triangle_materials[i].refraction_coefficient);
                            triangle_material.refraction_coefficient = glm::clamp(
                                triangle_material.refraction_coefficient, 0.0f,
                                1.0f - objects.triangle_materials[i].reflection_coefficient);

                            objects.triangle_materials[i] = triangle_material;
                        }

                        ImGui::TreePop();
                    }

                    ImGui::PopID();
                }

                ImGui::TreePop();
            }
            // CSG Sphere materials
            if (ImGui::TreeNode("CSG Sphere materials"))
            {
                // Show sphere materials
                for (int i = 0; i < MAX_CSG_SPHERES; i++)
                {
                    ImGui::PushID(i);
                    if (std::string label = std::format("CSG Sphere material {}", std::to_string(i + 1));
                        ImGui::TreeNode(label.c_str()))
                    {
                        auto& csg_sphere_material = objects.csg_sphere_materials[i];
                        bool changed = false;

                        changed |= ImGui::ColorEdit3("Diffuse", glm::value_ptr(csg_sphere_material.diffuse));
                        changed |= ImGui::ColorEdit3("Specular", glm::value_ptr(csg_sphere_material.specular));
                        changed |= ImGui::ColorEdit3("Ambient", glm::value_ptr(csg_sphere_material.ambient));
                        changed |= ImGui::DragFloat("Shininess", &csg_sphere_material.shininess, 1.0f, 0.0f);
                        changed |= ImGui::SliderFloat("Reflection coefficient",
                                                      &csg_sphere_material.reflection_coefficient, 0.0f, 1.0f);
                        changed |= ImGui::SliderFloat("Refraction coefficient",
                                                      &csg_sphere_material.refraction_coefficient, 0.0f, 1.0f);
                        changed |= ImGui::SliderFloat("Refraction index", &csg_sphere_material.refraction_index, 0.0f,
                                                      10.0f);
                        changed |= ImGui::SliderFloat("Glossiness", &csg_sphere_material.glossiness, 0.0f, 1.0f);
                        changed |= ImGui::ColorEdit3("Absorption", glm::value_ptr(csg_sphere_material.absorption));

                        if (changed)
                        {
                            csg_sphere_material.reflection_coefficient = glm::clamp(
                                csg_sphere_material.reflection_coefficient, 0.0f,
                                1.0f - objects.csg_sphere_materials[i].refraction_coefficient);
                            csg_sphere_material.refraction_coefficient = glm::clamp(
                                csg_sphere_material.refraction_coefficient, 0.0f,
                                1.0f - objects.csg_sphere_materials[i].reflection_coefficient);

                            objects.csg_sphere_materials[i] = csg_sphere_material;
                        }

                        ImGui::TreePop();
                    }

                    ImGui::PopID();
                }

                ImGui::TreePop();
            }

            ImGui::EndTabItem();
        }

        // About tab
        if (ImGui::BeginTabItem("About"))
        {
            ImGui::Text("Raytrace with ImGui controls");
            ImGui::Text("Press Tab to show/hide this UI");
            ImGui::Text("Press Escape to exit");

            ImGui::Separator();

            if (ImGui::Button("Reset to Default Scene"))
            {
                scene_data.reset_to_default();
            }

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();

    // Render ImGui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

gl3::renderer::renderer()
{
    init_window();

    // Initialize scene data
    scene_data.initialize();

    // Set up the camera to its initial position from scene data
    const auto& camera_data = scene_data.get_camera();
    camera.position = camera_data.position;
    camera.orientation = glm::normalize(camera_data.target - camera_data.position);

    // Create the shader program
    shader_program = std::make_unique<gl3::shader_class>("shaders/default.vert", "shaders/default.frag");

    // Create quad for rendering
    quad_VAO = std::make_unique<gl3::vao>();
    init_quad();

    // Initialize ImGui
    init_imgui();

    // Set the default clear color
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
}

gl3::renderer::~renderer()
{
    // Clean up the compute renderer
    compute_rend.reset();
    
    // Shutdown ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // Terminate GLFW
    glfwTerminate();
}

void gl3::renderer::render_frame()
{
    // Update viewport
    int width;
    int height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    // Clear buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // If in camera mode, update the scene camera from the interactive camera
    if (camera_mode)
    {
        camera.inputs(window);
        scene_data.get_camera().position = camera.position;
        scene_data.get_camera().target = camera.position + camera.orientation;
    }

    if (use_compute_shader)
    {
        // Initialize the compute renderer if it doesn't exist
        if (!compute_rend)
        {
            compute_rend = std::make_unique<compute_renderer>(scene_data, width, height);
        }

        // Render using the compute shader
        compute_rend->render();
        compute_rend->display();
    } else {
        // Update UBOs with current scene data
        scene_data.update_UBOs();

        // Activate shader and draw quad
        shader_program->activate();
        quad_VAO->bind();
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    // Render ImGui UI if enabled
    if (show_ui)
    {
        render_ui();
    }

    // Swap buffers and process events
    glfwSwapBuffers(window);
    glfwPollEvents();

    // Update FPS counter
    update_fps();
    std::stringstream ss;
    ss << WINDOW_TITLE << ": " << current_FPS << " fps";
    glfwSetWindowTitle(window, ss.str().c_str());
}

bool gl3::renderer::should_close() const
{
    return glfwWindowShouldClose(window);
}

void gl3::renderer::update_fps()
{
    frame_acc++;
    const double timeCurr = glfwGetTime();
    if (const double elapsedTime = timeCurr - prev_fps_update; elapsedTime > FPS_UPDATE_DELAY)
    {
        current_FPS = frame_acc / elapsedTime;
        frame_acc = 0;
        prev_fps_update = timeCurr;
    }
}
