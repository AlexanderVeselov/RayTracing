/*****************************************************************************
 MIT License

 Copyright(c) 2021 Alexander Veselov

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this softwareand associated documentation files(the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions :

 The above copyright noticeand this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 *****************************************************************************/

#include "gl_graphics_pipeline.hpp"
#include <cstring>
#include <stdexcept>
#include <fstream>

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

GraphicsPipeline::GraphicsPipeline(char const* vs_filename, char const* fs_filename)
{
    std::ifstream vs_file(vs_filename);
    if (!vs_file)
    {
        throw std::runtime_error("Failed to load vs file!");
    }

    std::string vs_source((std::istreambuf_iterator<char>(vs_file)), (std::istreambuf_iterator<char>()));

    std::ifstream fs_file(fs_filename);
    if (!fs_file)
    {
        throw std::runtime_error("Failed to load fs file!");
    }

    std::string fs_source((std::istreambuf_iterator<char>(fs_file)), (std::istreambuf_iterator<char>()));

    vertex_shader_ = CreateShader(vs_source.c_str(), GL_VERTEX_SHADER);
    fragment_shader_ = CreateShader(fs_source.c_str(), GL_FRAGMENT_SHADER);

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
