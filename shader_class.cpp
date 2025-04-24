#include <fstream>
#include <iostream>


#include "shader_class.h"
#include <GL/glew.h>


std::string get_file_contents(const char *filename) {
    if (std::ifstream in(filename, std::ios::binary); in) {
        std::string content;
        in.seekg(0, std::ios::end);
        content.resize(in.tellg());
        in.seekg(0, std::ios::beg);
        in.read(&content[0], content.size());
        in.close();
        return content;
    }
    throw std::runtime_error("Could not open file: " + std::string(filename));
}


gl3::shader_class::shader_class(const char *vertex_file, const char *fragment_file) {
    const std::string vertexCode = get_file_contents(vertex_file);
    const std::string fragmentCode = get_file_contents(fragment_file);

    const char *vertexSource = vertexCode.c_str();
    const char *fragmentSource = fragmentCode.c_str();

    // Create the Vertex Shader Object and get its reference
    std::cout << "Compiling vertex shader: " << vertex_file << "\n";
    const GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    // Attach the Vertex Shader source to the Vertex Shader Object
    glShaderSource(vertexShader, 1, &vertexSource, nullptr);
    // Compile the Vertex Shader into machine code
    glCompileShader(vertexShader);
    compile_errors(vertexShader, "VERTEX");
    // Create the Fragment Shader Object and get its reference
    std::cout << "Compiling fragment shader: " << fragment_file << "\n";
    const GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    // Attach the Fragment Shader source to the Fragment Shader Object
    glShaderSource(fragmentShader, 1, &fragmentSource, nullptr);
    // Compile the Vertex Shader into machine code
    glCompileShader(fragmentShader);
    compile_errors(fragmentShader, "FRAGMENT");
    // Create the Shader Program Object and get its reference
    std::cout << "Linking program\n";
    id = glCreateProgram();
    // Attach the Vertex and Fragment Shaders to the Shader Program
    glAttachShader(id, vertexShader);
    glAttachShader(id, fragmentShader);
    // Wrap-up/Link all the shaders together into the Shader Program
    glLinkProgram(id);
    compile_errors(id, "PROGRAM");
    std::cout << "Linking program ok\n";
    // Delete the now useless Vertex and Fragment Shader objects
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

gl3::shader_class::shader_class(const char* compute_file)
{
    const std::string compute_code = get_file_contents(compute_file);

    const char* compute_source = compute_code.c_str();

    // Create the compute shader and get its reference
    std::cout << "Compiling compute shader: " << compute_file << "\n";
    const GLuint computeShader = glCreateShader(GL_COMPUTE_SHADER);
    // Attach the compute shader source to the compute shader
    glShaderSource(computeShader, 1, &compute_source, nullptr);
    // Compile the compute shader into machine code
    glCompileShader(computeShader);
    compile_errors(computeShader, "COMPUTE");
    
    // Create the shader program and get its reference
    std::cout << "Linking program\n";
    id = glCreateProgram();
    // Attach the compute shader to the shader program
    glAttachShader(id, computeShader);
    // Wrap-up/Link all the shaders together into the shader program
    glLinkProgram(id);
    compile_errors(id, "PROGRAM");
    std::cout << "Linking program ok\n";
    // Delete the now useless compute shader object
    glDeleteShader(computeShader);
}

void gl3::shader_class::activate() const {
    glUseProgram(id);
}

void gl3::shader_class::deactivate()
{
    glUseProgram(0);
}

gl3::shader_class::~shader_class() {
    glDeleteProgram(id);
}

void gl3::shader_class::compile_errors(const unsigned int shader, const char *type) {
    GLint has_compiled;
    std::string info_log;
    info_log.resize(1024);
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &has_compiled);
        if (has_compiled == GL_FALSE) {
            glGetShaderInfoLog(shader, 1024, nullptr, info_log.data());
            std::cout << "SHADER_COMPILATION_ERROR for: " << type << "\n" << info_log << std::endl;
        }
    } else {
        glGetProgramiv(shader, GL_COMPILE_STATUS, &has_compiled);
        if (has_compiled == GL_FALSE) {
            glGetProgramInfoLog(shader, 1024, nullptr, info_log.data());
            std::cout << "SHADER_LINKING_ERROR for: " << type << "\n" << std::endl;
        }
    }
}