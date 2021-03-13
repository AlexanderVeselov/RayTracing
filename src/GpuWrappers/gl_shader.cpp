#include "gl_shader.hpp"
#include <cstring>
#include <stdexcept>

Shader::Shader(char const* source, GLenum shader_type)
{
    // Create shader
    shader_ = glCreateShader(shader_type);

    // Associate shader source
    GLint source_length = (GLint)strlen(source);
    glShaderSource(shader_, 1, &source, &source_length);

    // Compile shader
    glCompileShader(shader_);

    // Get compile status
    GLint compile_status;
    glGetShaderiv(shader_, GL_COMPILE_STATUS, &compile_status);

    // Throw an error with description if we can't compile the shader
    if (compile_status == false)
    {
        GLint log_length = 0;
        glGetShaderiv(shader_, GL_INFO_LOG_LENGTH, &log_length);

        // The maxLength includes the NULL character
        std::string info_log;
        info_log.resize(log_length);
        glGetShaderInfoLog(shader_, log_length, &log_length, &info_log[0]);

        throw std::runtime_error(info_log);
    }

}