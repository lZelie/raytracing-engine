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
        return (content);
    }
    throw (errno);
}


gl3::shader_class::shader_class(const char *vertexFile, const char *fragmentFile) {
    std::string vertexCode = get_file_contents(vertexFile);
    std::string fragmentCode = get_file_contents(fragmentFile);

    const char *vertexSource = vertexCode.c_str();
    const char *fragmentSource = fragmentCode.c_str();

    // Create Vertex Shader Object and get its reference
    std::cout << "Compiling vertex shader: " << vertexFile << "\n";
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    // Attach Vertex Shader source to the Vertex Shader Object
    glShaderSource(vertexShader, 1, &vertexSource, nullptr);
    // Compile the Vertex Shader into machine code
    glCompileShader(vertexShader);
    compileErrors(vertexShader, "VERTEX");
    // Create Fragment Shader Object and get its reference
    std::cout << "Compiling fragment shader: " << fragmentFile << "\n";
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    // Attach Fragment Shader source to the Fragment Shader Object
    glShaderSource(fragmentShader, 1, &fragmentSource, nullptr);
    // Compile the Vertex Shader into machine code
    glCompileShader(fragmentShader);
    compileErrors(fragmentShader, "FRAGMENT");
    // Create Shader Program Object and get its reference
    std::cout << "Linking program\n";
    id = glCreateProgram();
    // Attach the Vertex and Fragment Shaders to the Shader Program
    glAttachShader(id, vertexShader);
    glAttachShader(id, fragmentShader);
    // Wrap-up/Link all the shaders together into the Shader Program
    glLinkProgram(id);
    compileErrors(id, "PROGRAM");
    std::cout << "Linking program ok\n";
    // Delete the now useless Vertex and Fragment Shader objects
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
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

void gl3::shader_class::compileErrors(unsigned int shader, const char *type) {
    GLint hasCompiled;
    char infoLog[1024];
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &hasCompiled);
        if (hasCompiled == GL_FALSE) {
            glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
            std::cout << "SHADER_COMPILATION_ERROR for: " << type << "\n" << infoLog << std::endl;
        }
    } else {
        glGetProgramiv(shader, GL_COMPILE_STATUS, &hasCompiled);
        if (hasCompiled == GL_FALSE) {
            glGetProgramInfoLog(shader, 1024, nullptr, infoLog);
            std::cout << "SHADER_LINKING_ERROR for: " << type << "\n" << std::endl;
        }
    }
}