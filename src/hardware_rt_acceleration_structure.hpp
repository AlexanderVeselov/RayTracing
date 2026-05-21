/*****************************************************************************
 MIT License

 Copyright(c) 2026 Alexander Veselov

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

#include "acceleration_structure.hpp"
#include "gpu_acceleration_structure.hpp"

#include <vector>

class Scene;

namespace gpu
{
class CommandBuffer;
class Device;
}  // namespace gpu

class HardwareRtAccelerationStructure final : public AccelerationStructure
{
public:
    AccelerationStructureBackend GetBackend() const override { return AccelerationStructureBackend::kHardwareRt; }

    void Build(gpu::Device& device, gpu::CommandBuffer& command_buffer, Scene const& scene,
        gpu::BufferPtr const& vertex_buffer, gpu::BufferPtr const& index_buffer);

    gpu::AccelerationStructure& GetTopLevelAS() const { return *top_level_as_; }

private:
    std::vector<gpu::AccelerationStructurePtr> bottom_level_as_;
    gpu::AccelerationStructurePtr top_level_as_;
};
