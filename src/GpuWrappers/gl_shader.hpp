#ifndef GL_SHADER_HPP
#define GL_SHADER_HPP

#include <GL/glew.h>

class Shader
{
public:
    Shader(char const* source, GLenum shader_type);

private:
    GLuint shader_;

};

#endif // GL_SHADER_HPP
