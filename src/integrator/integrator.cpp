/*****************************************************************************
 MIT License

 Copyright(c) 2023 Alexander Veselov

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

#include "integrator.hpp"

void Integrator::Integrate()
{
    if (request_reset_ || enable_denoiser_)
    {
        Reset();
        request_reset_ = false;
    }

    GenerateRays();

    for (std::uint32_t bounce = 0; bounce <= max_bounces_; ++bounce)
    {
        IntersectRays(bounce);
        if (bounce == 0)
        {
            ComputeAOVs();
        }
        ShadeMissedRays(bounce);
        ClearOutgoingRayCounter(bounce);
        ClearShadowRayCounter();
        ShadeSurfaceHits(bounce);
        IntersectShadowRays();
        AccumulateDirectSamples();
    }

    AdvanceSampleCount();
    if (enable_denoiser_)
    {
        Denoise();
        CopyHistoryBuffers();
    }
    ResolveRadiance();
}

void Integrator::SetMaxBounces(std::uint32_t max_bounces)
{
    max_bounces_ = max_bounces;
    RequestReset();
}

void Integrator::EnableWhiteFurnace(bool enable)
{
    if (enable == enable_white_furnace_)
    {
        return;
    }

    enable_white_furnace_ = enable;
    CreateKernels();
    RequestReset();
}
