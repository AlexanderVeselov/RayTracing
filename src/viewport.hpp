#ifndef m_ViewportHPP
#define m_ViewportHPP

#include "cl_context.hpp"
#include "camera.hpp"
#include <CL/cl.h>

class Viewport
{
public:
    Viewport(const Camera& camera, size_t width, size_t height, size_t rectCount);
    void Draw() const;
    void Update(ClContext &context);

private:
    const Camera& m_Camera;
    size_t m_Width;
    size_t m_Height;
    size_t m_RectCount;
    size_t m_CurrentRect;
    size_t m_FrameCount;
    cl_float4* m_Pixels;

};

#endif // m_ViewportHPP
