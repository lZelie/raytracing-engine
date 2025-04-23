#ifndef SHADER_CLASS_H
#define SHADER_CLASS_H

#include <string>
#include <GL/glew.h>


std::string get_file_contents(const char* filename);

namespace gl3
{
    class shader_class
    {
    public:
        GLuint id;

        shader_class(const char* vertexFile, const char* fragmentFile);

        void activate() const;
        static void deactivate();

        ~shader_class();

    public:
        static void compileErrors(unsigned int shader, const char* type);
    };
}


#endif //SHADER_CLASS_H
