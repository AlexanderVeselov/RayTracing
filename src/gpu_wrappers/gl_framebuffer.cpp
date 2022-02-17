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

#pragma once

#include "gl_framebuffer.hpp"
#include <cassert>

GLFramebuffer::GLFramebuffer(std::uint32_t width, std::uint32_t height)
    : width_(width)
    , height_(height)
{
    CreateFramebuffer();
}

GLFramebuffer::~GLFramebuffer()
{
    DestroyFramebuffer();
}

void GLFramebuffer::CreateFramebuffer()
{
    // Create a framebuffer texture
    glCreateTextures(GL_TEXTURE_2D, 1, &framebuffer_texture_);
    glTextureStorage2D(framebuffer_texture_, 1, GL_R32UI, width_, height_);
    glTextureParameteri(framebuffer_texture_, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(framebuffer_texture_, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    // Create a new framebuffer
    glCreateFramebuffers(1, &framebuffer_);
    // Bind framebuffer texture
    glNamedFramebufferTexture(framebuffer_, GL_COLOR_ATTACHMENT0, framebuffer_texture_, 0);

    // Create a renderbuffer for depth
    glCreateRenderbuffers(1, &depth_renderbuffer_);
    glNamedRenderbufferStorage(depth_renderbuffer_, GL_DEPTH24_STENCIL8, width_, height_);
    glNamedFramebufferRenderbuffer(framebuffer_, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth_renderbuffer_);
}

void GLFramebuffer::DestroyFramebuffer()
{
    assert(framebuffer_ && framebuffer_texture_ && depth_renderbuffer_);
    glDeleteRenderbuffers(1, &depth_renderbuffer_);
    glDeleteFramebuffers(1, &framebuffer_);
    glDeleteTextures(1, &framebuffer_texture_);
}

void GLFramebuffer::Resize(std::uint32_t width, std::uint32_t height)
{
    if (width_ == width && height_ == height)
    {
        return;
    }

    width_ = width;
    height_ = height;

    DestroyFramebuffer();
    CreateFramebuffer();
}
