/*****************************************************************************
 MIT License

 Copyright(c) 2022 Alexander Veselov

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
#include "gl_shader_utils.hpp"
#include <cstring>
#include <stdexcept>
#include <fstream>

GraphicsPipeline::GraphicsPipeline(char const* vs_filename, char const* fs_filename)
{
    vertex_shader_ = CreateShader(vs_filename, GL_VERTEX_SHADER);
    fragment_shader_ = CreateShader(fs_filename, GL_FRAGMENT_SHADER);

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
