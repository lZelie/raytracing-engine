cmake_minimum_required(VERSION 3.31)
project(Raytracing1)

set(CMAKE_CXX_STANDARD 23)

# Set this globally to affect all library fetching
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)

#OpenGL
find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIRS})

#GLFW
find_package(glfw QUIET)
if (NOT glfw_found)
    include(FetchContent)
    FetchContent_Declare(GLFW GIT_REPOSITORY https://github.com/glfw/glfw.git)
    # Force GLFW to build only the static library
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(GLFW)
endif ()

# GLM
find_package(glm CONFIG QUIET)
if (NOT glm_FOUND)
    include(FetchContent)
    FetchContent_Declare(GLM GIT_REPOSITORY https://github.com/icaven/glm.git)
    FetchContent_MakeAvailable(GLM)
endif ()

# GLEW - Custom handling to ensure static linking
include(FetchContent)
FetchContent_Declare(
        GLEW
        URL https://sourceforge.net/projects/glew/files/glew/2.1.0/glew-2.1.0.zip
        SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/glew
)
FetchContent_GetProperties(GLEW)
if(NOT glew_POPULATED)
    FetchContent_MakeAvailable(GLEW)

    # We'll build GLEW manually to ensure it's built statically
    file(GLOB GLEW_SOURCE_FILES "${CMAKE_SOURCE_DIR}/external/glew/src/*.c")
    add_library(glew_static STATIC ${GLEW_SOURCE_FILES})
    target_include_directories(glew_static PUBLIC ${CMAKE_SOURCE_DIR}/external/glew/include)

    if(WIN32)
        target_link_libraries(glew_static PUBLIC opengl32 glu32)
        target_compile_definitions(glew_static PUBLIC -DGLEW_STATIC -DGLEW_NO_GLU)
    else()
        target_link_libraries(glew_static PUBLIC ${OPENGL_LIBRARIES})
        target_compile_definitions(glew_static PUBLIC -DGLEW_STATIC)
    endif()
endif()

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
        bvh.cpp
        bvh.h
)
target_include_directories(Raytracing1 PRIVATE "${CMAKE_SOURCE_DIR}/include" ${CMAKE_SOURCE_DIR}/external/glew/include)
target_link_libraries(Raytracing1 ${OPENGL_LIBRARY} glfw glm::glm-header-only glew_static imgui)

# Apply static linking for C++ runtime libraries to the final executable
if(MSVC)
    target_compile_options(Raytracing1 PRIVATE /MT$<$<CONFIG:Debug>:d>)
    set_property(TARGET Raytracing1 PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
elseif(MINGW)
    target_link_options(Raytracing1 PRIVATE -static -static-libgcc -static-libstdc++)
elseif(UNIX AND NOT APPLE)
    target_link_options(Raytracing1 PRIVATE -static-libgcc -static-libstdc++)
endif()

# Copy shaders to the target output directory
add_custom_command(
        TARGET Raytracing1 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:Raytracing1>/shaders
        COMMENT "Copying shader files to build directory"
)