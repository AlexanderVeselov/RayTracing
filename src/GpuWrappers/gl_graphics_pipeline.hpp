#ifndef GL_GRAPHICS_PIPELINE_HPP
#define GL_GRAPHICS_PIPELINE_HPP

#include <GL/glew.h>

class GraphicsPipeline
{
public:
    GraphicsPipeline(char const* vs_source, char const* fs_source);
    void Use() const { glUseProgram(shader_program_); };

private:
    GLuint vertex_shader_;
    GLuint fragment_shader_;
    GLuint shader_program_;

};

#endif // GL_GRAPHICS_PIPELINE_HPP
