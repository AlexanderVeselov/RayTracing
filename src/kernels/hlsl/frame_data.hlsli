#ifndef FRAME_DATA_HLSLI
#define FRAME_DATA_HLSLI

cbuffer FrameData : register(b1)
{
    float4 g_CameraPositionFov;
    float4 g_CameraFrontAspect;
    float4 g_CameraUpAperture;
    float4 g_CameraLens;
    float4 g_PrevCameraPositionFov;
    float4 g_PrevCameraFrontAspect;
    float4 g_PrevCameraUpAperture;
    float4 g_PrevCameraLens;
    uint4 g_RenderSize;
    uint4 g_SceneCounts;
    uint4 g_RenderParams;
};

static const uint SHADED_COLOR_INDEX = 0u;
static const uint DIFFUSE_INDEX = 1u;
static const uint DEPTH_INDEX = 2u;
static const uint NORMAL_INDEX = 3u;
static const uint MOTION_VECTORS_INDEX = 4u;

static const uint RENDER_FLAG_WHITE_FURNACE = 1u;
static const uint RENDER_FLAG_DENOISER = 2u;

#endif
