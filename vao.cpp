#include "vao.h"

#include <GL/glew.h>

#include "vbo.h"


gl3::vao::vao() {
    glGenVertexArrays(1, &ID);
}

void gl3::vao::linkAttrib(vbo &VBO, GLuint layout, GLuint numComponent, GLenum type, GLsizeiptr stride, void *offset) {
    VBO.bind();
    glEnableVertexAttribArray(layout);
    glVertexAttribPointer(layout, numComponent, type, GL_FALSE, stride, offset);
    VBO.unbind();
}

void gl3::vao::bind() {
    glBindVertexArray(ID);
}

void gl3::vao::unbind() {
    glBindVertexArray(0);
}

gl3::vao::~vao() {
    glDeleteVertexArrays(1, &ID);
}