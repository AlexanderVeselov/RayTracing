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

#include "gl_shader_utils.hpp"
#include <string>
#include <stdexcept>
#include <fstream>

std::string ReadFile(char const* filename)
{
    std::ifstream file(filename);
    if (!filename)
    {
        throw std::runtime_error("Failed to load shader from file!");
    }

    return std::string((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));
}

GLuint CreateShader(char const* filename, GLenum shader_type)
{
    std::string source = ReadFile(filename);
    char const* source_c_str = source.c_str();

    // Create shader
    GLuint shader = glCreateShader(shader_type);

    // Associate shader source
    GLint source_length = source.size();
    glShaderSource(shader, 1, &source_c_str, &source_length);

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
