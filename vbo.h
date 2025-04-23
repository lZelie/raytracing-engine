#ifndef GL2_VBO_H
#define GL2_VBO_H


#include <vector>
#include <glm/vec4.hpp>
#include <GL/glew.h>


namespace gl3 {
    class vbo {
    public:
        GLuint ID;

        vbo(const std::vector<glm::vec4>& positionsColors) {
            glGenBuffers(1, &ID);
            glBindBuffer(GL_ARRAY_BUFFER, ID);
            glBufferData(GL_ARRAY_BUFFER, positionsColors.size() * sizeof (glm::vec4 ), positionsColors.data(), GL_STATIC_DRAW);
        }

        vbo(const std::vector<GLfloat>& data) {
            glGenBuffers(1, &ID);
            glBindBuffer(GL_ARRAY_BUFFER, ID);
            glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof (GLfloat ), data.data(), GL_STATIC_DRAW);
        }

        vbo(const std::vector<glm::vec4>& positions, const std::vector<glm::vec4>& colors) {
            glGenBuffers(1, &ID);
            glBindBuffer(GL_ARRAY_BUFFER, ID);
            const int sp = positions.size() * sizeof(glm::vec4);
            const int sc = colors.size() * sizeof(glm::vec4);
            glBufferData(GL_ARRAY_BUFFER, sp + sc, nullptr, GL_STATIC_DRAW);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sp, positions.data());
            glBufferSubData(GL_ARRAY_BUFFER, sp, sc, colors.data());
        }

        void bind();

        void unbind();

        ~vbo();
    };
}


#endif //GL2_VBO_H
