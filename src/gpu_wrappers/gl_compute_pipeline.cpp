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

#include "gl_compute_pipeline.hpp"
#include "gl_shader_utils.hpp"
#include <cstring>
#include <stdexcept>

ComputePipeline::ComputePipeline(char const* cs_source)
{
    shader_ = CreateShader(cs_source, GL_COMPUTE_SHADER);

    shader_program_ = glCreateProgram();
    glAttachShader(shader_program_, shader_);
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

ComputePipeline::~ComputePipeline()
{
    glDeleteProgram(shader_program_);
    glDeleteShader(shader_);
}

void ComputePipeline::BindConstant(char const* name, std::uint32_t value)
{
    GLuint uniform_index = glGetUniformLocation(shader_program_, name);
    if (uniform_index == GL_INVALID_INDEX)
    {
        throw std::runtime_error(("Can't find variable " + std::string(name)).c_str());
    }
    glUniform1ui(uniform_index, value);
}

void ComputePipeline::BindConstant(char const* name, float value)
{
    GLuint uniform_index = glGetUniformLocation(shader_program_, name);
    if (uniform_index == GL_INVALID_INDEX)
    {
        throw std::runtime_error(("Can't find variable " + std::string(name)).c_str());
    }
    glUniform1f(uniform_index, value);
}

void ComputePipeline::BindConstant(char const* name, float3 value)
{
    GLuint uniform_index = glGetUniformLocation(shader_program_, name);
    if (uniform_index == GL_INVALID_INDEX)
    {
        throw std::runtime_error(("Can't find variable " + std::string(name)).c_str());
    }
    glUniform3f(uniform_index, value.x, value.y, value.z);
}
