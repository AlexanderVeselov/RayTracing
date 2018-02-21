#ifndef m_CameraHPP
#define m_CameraHPP

#include "mathlib.hpp"
#include "viewport.hpp"
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

private:
    std::shared_ptr<Viewport> m_Viewport;

    float3 m_Origin;
    float3 m_Velocity;
    float3 m_Front;
    float3 m_Up;

    float m_Pitch;
    float m_Yaw;    
    float m_Speed;

    bool m_Changed;

};

#endif // m_CameraHPP
