#ifndef CAMERA_HPP
#define CAMERA_HPP

#include "mathlib/mathlib.hpp"
#include "utils/viewport.hpp"
#include <memory>

class Camera
{
public:
    Camera(std::shared_ptr<Viewport> viewport);
    void Update();

    float3 GetOrigin()      const { return m_Origin; }
    float3 GetFrontVector() const { return m_Front; }
    float3 GetUpVector()    const { return m_Up; }

    bool IsChanged()        const { return m_Changed; }
    unsigned int GetFrameCount() const { return m_FrameCount; }

private:
    std::shared_ptr<Viewport> m_Viewport;

    float3 m_Origin;
    float3 m_Velocity;
    float3 m_Front;
    float3 m_Up;

    float m_Pitch;
    float m_Yaw;    
    float m_Speed;

    unsigned int m_FrameCount;

    bool m_Changed;

};

#endif // CAMERA_HPP
