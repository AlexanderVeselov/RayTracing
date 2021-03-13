#include "gl_graphics_pipeline.hpp"
#include <cstring>
#include <stdexcept>

namespace
{
    GLuint CreateShader(char const* source, GLenum shader_type)
    {
        // Create shader
        GLuint shader = glCreateShader(shader_type);

        // Associate shader source
        GLint source_length = (GLint)strlen(source);
        glShaderSource(shader, 1, &source, &source_length);

        // Compile shader
        glCompileShader(shader);

        // Get compile status
        GLint compile_status;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_status);

        // Throw an error with description if we can't compile the shader
        if (compile_status == false)
        {
            GLint log_length = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);

            // The maxLength includes the NULL character
            std::string info_log;
            info_log.resize(log_length);
            glGetShaderInfoLog(shader, log_length, &log_length, &info_log[0]);

            throw std::runtime_error(info_log);
        }

        return shader;

    }
}

GraphicsPipeline::GraphicsPipeline(char const* vs_source, char const* fs_source)
{
    vertex_shader_ = CreateShader(vs_source, GL_VERTEX_SHADER);
    fragment_shader_ = CreateShader(fs_source, GL_FRAGMENT_SHADER);

    shader_program_ = glCreateProgram();
    glAttachShader(shader_program_, vertex_shader_);
    glAttachShader(shader_program_, fragment_shader_);
    glLinkProgram(shader_program_);

    GLint link_status;
    glGetProgramiv(shader_program_, GL_LINK_STATUS, &link_status);

    if (link_status == false)
    {
        GLint log_length = 0;
        glGetProgramiv(shader_program_, GL_INFO_LOG_LENGTH, &log_length);

        // The maxLength includes the NULL character
        std::string info_log;
        info_log.resize(log_length);
        glGetProgramInfoLog(shader_program_, log_length, &log_length, &info_log[0]);

        throw std::runtime_error(info_log);
    }

}

GraphicsPipeline::~GraphicsPipeline()
{
    glDeleteProgram(shader_program_);
    glDeleteShader(fragment_shader_);
    glDeleteShader(vertex_shader_);
}
