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
#include <vector>
#include <stack>
#include <stdexcept>
#include <fstream>

std::string ReadHeader(char const* filename)
{
    std::ifstream file(filename);
    if (!file)
    {
        return "";
    }

    std::string source;
    std::string line;
    while (std::getline(file, line))
    {
        std::string include_str("#include");

        std::size_t start_pos = 0;
        if ((start_pos = line.find(include_str)) != std::string::npos)
        {
            start_pos = line.find("\"", start_pos) + 1;
            std::size_t end_pos = line.find("\"", start_pos);

            std::string include_filename = line.substr(start_pos, end_pos - start_pos);
            source += ReadHeader(include_filename.c_str()) + "\n";
        }
        else
        {
            source += line + "\n";
        }
    }

    return source;
}

std::string ReadShader(char const* filename)
{
    std::ifstream file(filename);
    if (!file)
    {
        throw std::runtime_error("Failed to load shader!");
    }

    std::string source;
    source += "#version 430 core\n";
    source += "#define GLSL\n";
    source += "#define typedef\n";
    source += "#define float4 vec4\n";
    source += "#define float3 vec3\n";
    source += "#define float2 vec2\n";

    std::string line;
    while (std::getline(file, line))
    {
        std::string include_str("#include");

        std::size_t start_pos = 0;
        if ((start_pos = line.find(include_str)) != std::string::npos)
        {
            start_pos = line.find("\"", start_pos) + 1;
            std::size_t end_pos = line.find("\"", start_pos);

            std::string include_filename = line.substr(start_pos, end_pos - start_pos);
            source += ReadHeader(include_filename.c_str()) + "\n";
        }
        else
        {
            source += line + "\n";
        }
    }

    return source;
}

GLuint CreateShader(char const* filename, GLenum shader_type)
{
    std::string source = ReadShader(filename);
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
