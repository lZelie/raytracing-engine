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

        // Constructor for graphics shaders (vertex + fragment)
        shader_class(const char* vertex_file, const char* fragment_file);

        // Constructor for compute shaders
        explicit shader_class(const char* compute_file);

        void activate() const;
        static void deactivate();

        ~shader_class();

        static void compile_errors(unsigned int shader, const char* type);
    };
}


#endif //SHADER_CLASS_H
