#include "render.hpp"
#include "mathlib.hpp"
#include "inputsystem.hpp"
#include "exception.hpp"
#include "hdr.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <gl/GL.h>
#include <gl/GLU.h>

#pragma comment( lib, "glu32.lib" )

static Render g_Render;
Render* render = &g_Render;

/*
GLuint ImageUtils::loadDDS(const char * filename)
{
    Logger::getInstance()->write(StringUtils::format("Loading DDS Image %s", filename));

    unsigned char header[124];

    FILE *fp;

    fp = fopen(filename, "rb");
    if (fp == NULL)
    {
        Logger::getInstance()->write("Failed to load Image: could not open the file");
        return 0;
    }

    char filecode[4];
    fread(filecode, 1, 4, fp);
    if (strncmp(filecode, "DDS ", 4) != 0)
    {
        fclose(fp);
        Logger::getInstance()->write("Failed to load Image: not a direct draw surface file");
        return 0;
    }

    fread(&header, 124, 1, fp);

    unsigned int height = *(unsigned int*)&(header[8]);
    unsigned int width = *(unsigned int*)&(header[12]);
    unsigned int linearSize = *(unsigned int*)&(header[16]);
    unsigned int mipMapCount = *(unsigned int*)&(header[24]);
    unsigned int fourCC = *(unsigned int*)&(header[80]);

    unsigned char * buffer;
    unsigned int bufsize;

    bufsize = mipMapCount > 1 ? linearSize * 2 : linearSize;
    buffer = (unsigned char*)malloc(bufsize * sizeof(unsigned char));
    fread(buffer, 1, bufsize, fp);

    fclose(fp);

    unsigned int components = (fourCC == FOURCC_DXT1) ? 3 : 4;
    unsigned int format;
    switch (fourCC)
    {
    case FOURCC_DXT1:
        format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
        break;
    case FOURCC_DXT3:
        format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
        break;
    case FOURCC_DXT5:
        format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        break;
    default:
        free(buffer);
        throw Exception("Failed to load Image: dds file format not supported (supported formats: DXT1, DXT3, DXT5)");
        return 0;
    }

    // Create one OpenGL texture
    GLuint textureID;
    glGenTextures(1, &textureID);

    // "Bind" the newly created texture : all future texture functions will modify this texture
    glBindTexture(GL_TEXTURE_2D, textureID);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    unsigned int blockSize = (format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) ? 8 : 16;
    unsigned int offset = 0;

    for (unsigned int level = 0; level < mipMapCount && (width || height); ++level)
    {
        unsigned int size = ((width + 3) / 4)*((height + 3) / 4)*blockSize;
        glCompressedTexImage2D(GL_TEXTURE_2D, level, format, width, height,
            0, size, buffer + offset);

        offset += size;
        width /= 2;
        height /= 2;
    }

    free(buffer);

    //Unbind the texture
    glBindTexture(GL_TEXTURE_2D, 0);

    Logger::getInstance()->write(StringUtils::format("Loaded DDS Image %s", filename));

    return textureID;
}
*/

void Render::InitGL()
{
    PIXELFORMATDESCRIPTOR pfd;
    ZeroMemory(&pfd, sizeof(PIXELFORMATDESCRIPTOR));
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 32;
    pfd.cStencilBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int pixelFormat = ChoosePixelFormat(m_DisplayContext, &pfd);
    SetPixelFormat(m_DisplayContext, pixelFormat, &pfd);

    m_GLContext = wglCreateContext(m_DisplayContext);
    wglMakeCurrent(m_DisplayContext, m_GLContext);

    // Disable VSync
    typedef BOOL(APIENTRY * wglSwapIntervalEXT_Func)(int);
    wglSwapIntervalEXT_Func wglSwapIntervalEXT = wglSwapIntervalEXT_Func(wglGetProcAddress("wglSwapIntervalEXT"));
    if (wglSwapIntervalEXT)
    {
        wglSwapIntervalEXT(0);
    }
}

void Render::Init(HWND hwnd)
{
    m_hWnd = hwnd;
    m_DisplayContext = GetDC(m_hWnd);
    
    InitGL();

    RECT hwndRect;
    GetWindowRect(hwnd, &hwndRect);

    unsigned int width = hwndRect.right - hwndRect.left, height = hwndRect.bottom - hwndRect.top;
    m_Viewport = std::make_shared<Viewport>(0, 0, width, height);
    m_Camera = std::make_shared<Camera>(m_Viewport);
#ifdef BVH_INTERSECTION
    m_Scene = std::make_shared<BVHScene>("meshes/dragon.obj", 4);
#else
    m_Scene = std::make_shared<UniformGridScene>("meshes/room.obj");
#endif
    
    std::vector<cl::Platform> all_platforms;
    cl::Platform::get(&all_platforms);
    if (all_platforms.empty())
    {
        throw Exception("No OpenCL platforms found");
    }
    
    m_CLContext = std::make_shared<CLContext>(all_platforms[0]);

    std::vector<cl::Device> platform_devices;
    all_platforms[0].getDevices(CL_DEVICE_TYPE_ALL, &platform_devices);
#ifdef BVH_INTERSECTION
    m_RenderKernel = std::make_shared<CLKernel>("src/kernel_bvh.cl", platform_devices);
#else
    m_RenderKernel = std::make_shared<CLKernel>("src/kernel_grid.cl", platform_devices);
#endif

    SetupBuffers();
    
}

HDRImage image;
void Render::SetupBuffers()
{
    GetCLKernel()->SetArgument(RenderKernelArgument_t::WIDTH, &m_Viewport->width, sizeof(size_t));
    GetCLKernel()->SetArgument(RenderKernelArgument_t::HEIGHT, &m_Viewport->height, sizeof(size_t));

    cl_int errCode;
    m_OutputBuffer = cl::Buffer(GetCLContext()->GetContext(), CL_MEM_READ_WRITE, GetGlobalWorkSize() * sizeof(float3), 0, &errCode);
    if (errCode)
    {
        throw CLException("Failed to create output buffer", errCode);
    }
    GetCLKernel()->SetArgument(RenderKernelArgument_t::BUFFER_OUT, &m_OutputBuffer, sizeof(cl::Buffer));
    
    m_Scene->SetupBuffers();

    // Texture Buffers
    cl::ImageFormat imageFormat;
    imageFormat.image_channel_order = CL_RGBA;
    imageFormat.image_channel_data_type = CL_FLOAT;

    HDRLoader::Load("textures/CGSkies_0036_free.hdr", image);
    
    m_Texture0 = cl::Image2D(GetCLContext()->GetContext(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, imageFormat, image.width, image.height, 0, image.colors, &errCode);
    if (errCode)
    {
        throw CLException("Failed to create image", errCode);
    }
    GetCLKernel()->SetArgument(RenderKernelArgument_t::TEXTURE0, &m_Texture0, sizeof(cl::Image2D));

}

const HWND Render::GetHWND() const
{
    return m_hWnd;
}

double Render::GetCurtime() const
{
    return (double)clock() / (double)CLOCKS_PER_SEC;
}

double Render::GetDeltaTime() const
{
    return GetCurtime() - m_PreviousFrameTime;
}

unsigned int Render::GetGlobalWorkSize() const
{
    if (m_Viewport)
    {
        return m_Viewport->width * m_Viewport->height;
    }
    else
    {
        return 0;
    }
}

HDC Render::GetDisplayContext() const
{
    return m_DisplayContext;
}

HGLRC Render::GetGLContext() const
{
    return m_GLContext;
}

std::shared_ptr<CLContext> Render::GetCLContext() const
{
    return m_CLContext;
}

std::shared_ptr<CLKernel> Render::GetCLKernel() const
{
    return m_RenderKernel;
}

void Render::FrameBegin()
{
    m_StartFrameTime = GetCurtime();
}

void Render::FrameEnd()
{
    m_PreviousFrameTime = m_StartFrameTime;
}

void Render::RenderFrame()
{
    FrameBegin();

    glClearColor(0.0f, 0.5f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    m_Camera->Update();

    //if (m_Camera->GetFrameCount() > 50) return;

    unsigned int globalWorksize = GetGlobalWorkSize();
    GetCLContext()->ExecuteKernel(GetCLKernel(), globalWorksize);
    GetCLContext()->ReadBuffer(m_OutputBuffer, m_Viewport->pixels, sizeof(float3) * globalWorksize);
    GetCLContext()->Finish();

    glDrawPixels(m_Viewport->width, m_Viewport->height, GL_RGBA, GL_FLOAT, m_Viewport->pixels);
        
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(90.0, 1280.0 / 720.0, 1, 1024);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    float3 eye = m_Camera->GetOrigin();
    float3 center = m_Camera->GetFrontVector() + eye;
    gluLookAt(eye.x, eye.y, eye.z, center.x, center.y, center.z, m_Camera->GetUpVector().x, m_Camera->GetUpVector().y, m_Camera->GetUpVector().z);

    //m_Scene->DrawDebug();

    glFinish();

    SwapBuffers(m_DisplayContext);

    FrameEnd();
}

void Render::Shutdown()
{

}
