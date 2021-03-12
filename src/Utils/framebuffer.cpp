#include "framebuffer.hpp"

#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <gl/GL.h>
#include <gl/GLU.h>

#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31
#define GL_COMPILE_STATUS 0x8B81

namespace
{
    char const* kVertexShaderSource = "chay";
    char const* kFragmentShaderSource = "kalka";
}

Framebuffer::Framebuffer(std::uint32_t width, std::uint32_t height)
    : width_(width)
    , height_(height)
{
    // Create framebuffer texture
    std::vector<std::uint32_t> tex_data(width_ * height_, 0xFFFFFFFF);

    GLuint render_texture;
    glGenTextures(1, &render_texture);
    glBindTexture(GL_TEXTURE_2D, render_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width_, height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex_data.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    // Load shader functions
    using glCreateShader_Func = GLuint(*)(GLenum);
    glCreateShader_Func glCreateShader = (glCreateShader_Func)wglGetProcAddress("glCreateShader");

    if (!glCreateShader)
    {
        throw std::runtime_error("Failed to find glCreateShader function!");
    }

    using glShaderSource_Func = void(*)(GLuint, GLsizei, const char**, const GLint*);
    glShaderSource_Func glShaderSource = (glShaderSource_Func)wglGetProcAddress("glShaderSource");

    if (!glShaderSource)
    {
        throw std::runtime_error("Failed to find glShaderSource function!");
    }

    using glCompileShader_Func = void(*)(GLuint);
    glCompileShader_Func glCompileShader = (glCompileShader_Func)wglGetProcAddress("glCompileShader");

    if (!glCompileShader)
    {
        throw std::runtime_error("Failed to find glCompileShader function!");
    }

    using glGetShaderiv_Func = void(*)(GLuint, GLenum, GLint*);
    glGetShaderiv_Func glGetShaderiv = (glGetShaderiv_Func)wglGetProcAddress("glGetShaderiv");

    if (!glGetShaderiv)
    {
        throw std::runtime_error("Failed to find glGetShaderiv function!");
    }

    using glGetShaderInfoLog_Func = void(*)(GLuint, GLsizei, GLsizei, char*);
    glGetShaderInfoLog_Func glGetShaderInfoLog = (glGetShaderInfoLog_Func)wglGetProcAddress("glGetShaderInfoLog");

    if (!glGetShaderInfoLog)
    {
        throw std::runtime_error("Failed to find glGetShaderInfoLog function!");
    }

    // Create shaders
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

    // Associate shader source
    GLint vs_source_length = (GLint)strlen(kVertexShaderSource);
    GLint fs_source_length = (GLint)strlen(kFragmentShaderSource);
    glShaderSource(vertex_shader, 1, &kVertexShaderSource, &vs_source_length);
    glShaderSource(fragment_shader, 1, &kFragmentShaderSource, &fs_source_length);

    // Compile shaders
    glCompileShader(vertex_shader);
    glCompileShader(fragment_shader);

    // Get compile status
    GLint compile_status;
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &compile_status);

    if (compile_status == false)
    {
        GLint maxLength = 0;
        glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &maxLength);

        // The maxLength includes the NULL character
        std::vector<GLchar> infoLog(maxLength);
        glGetShaderInfoLog(vertexShader, maxLength, &maxLength, &infoLog[0]);

    }
}

Framebuffer::~Framebuffer()
{

}
