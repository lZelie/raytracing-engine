#ifndef GL2_VAO_H
#define GL2_VAO_H


#include "vbo.h"

namespace gl3 {
    class vao {
    public:
        GLuint ID;

        vao();

        void linkAttrib(vbo &VBO, GLuint layout, GLuint numComponent, GLenum type, GLsizeiptr stride, void *offset);

        void bind();

        void unbind();

        ~vao();
    };
}


#endif //GL2_VAO_H
