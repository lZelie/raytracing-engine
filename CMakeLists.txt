cmake_minimum_required(VERSION 3.31)
project(Raytracing1)

set(CMAKE_CXX_STANDARD 23)

#OpenGL
find_package(OpenGL REQUIRED)
include_directories( ${OPENGL_INCLUDE_DIRS} )

#GLFW
find_package(glfw QUIET)
if (NOT glfw_found)
    include(FetchContent)
    FetchContent_Declare(GLFW GIT_REPOSITORY https://github.com/glfw/glfw.git)
    FetchContent_MakeAvailable(GLFW)
endif ()

# GLM
find_package(glm CONFIG QUIET)
if (NOT glm_FOUND)
    include(FetchContent)
    FetchContent_Declare(GLM GIT_REPOSITORY https://github.com/icaven/glm.git)
    FetchContent_MakeAvailable(GLM)
endif ()

# GLEW
find_package(GLEW QUIET)
if (NOT GLEW_FOUND)
    include(FetchContent)
    SET(BUILD_UTILS OFF)
    FetchContent_Declare(
            GLEW
            URL            https://sourceforge.net/projects/glew/files/glew/2.1.0/glew-2.1.0.zip
            SOURCE_DIR     ${CMAKE_SOURCE_DIR}/external/glew
    )
    FetchContent_GetProperties(glew)
    if(NOT glew_POPULATED)
        FetchContent_MakeAvailable(glew)
        add_subdirectory(${CMAKE_SOURCE_DIR}/external/glew/build/cmake)
    endif()

    # Create an alias target to make linking consistent
    if(TARGET glew AND NOT TARGET GLEW::GLEW)
        add_library(GLEW::GLEW ALIAS glew)
    endif()
    if(TARGET glew_s AND NOT TARGET GLEW::GLEW_S)
        add_library(GLEW::GLEW_S ALIAS glew_s)
    endif()
endif (NOT GLEW_FOUND)

# ImGui
include(FetchContent)
FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui
        GIT_TAG v1.91.9b
)
FetchContent_MakeAvailable(imgui)

# Create ImGui library
set(IMGUI_SOURCES
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui PRIVATE glfw)

add_executable(Raytracing1 main.cpp
        shader_class.cpp
        shader_class.h
        vao.h
        vao.cpp
        vbo.cpp
        vbo.h
        camera.cpp
        camera.h
        Renderer.cpp
        Renderer.h
        scene_data.cpp
        scene_data.h
        compute_renderer.cpp
        compute_renderer.h
)
target_include_directories(Raytracing1 PRIVATE "${CMAKE_SOURCE_DIR}/include" external/glew/include)
target_link_libraries(Raytracing1 ${OPENGL_LIBRARY} glfw glm::glm-header-only GLEW::GLEW_S imgui)

# Copy shaders to the target output directory (preserving your existing build directory structure)
add_custom_command(
        TARGET Raytracing1 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:Raytracing1>/shaders
        COMMENT "Copying shader files to build directory"
)