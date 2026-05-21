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

#include "hardware_rt_acceleration_structure.hpp"

#include "gpu_command_buffer.hpp"
#include "gpu_device.hpp"
#include "scene/scene.hpp"

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace
{
void StoreTransform3x4(glm::mat4 const& transform, float out_transform[3][4])
{
    // D3D12/Vulkan instance descriptors both store a row-major 3x4 object-to-world transform.
    for (uint32_t row = 0; row < 3; ++row)
    {
        for (uint32_t column = 0; column < 4; ++column)
        {
            out_transform[row][column] = transform[column][row];
        }
    }
}
}  // namespace

void HardwareRtAccelerationStructure::Build(gpu::Device& device, gpu::CommandBuffer& command_buffer, Scene const& scene,
    gpu::BufferPtr const& vertex_buffer, gpu::BufferPtr const& index_buffer)
{
    auto const& mesh_infos = scene.GetMeshInfos();
    auto const& instance_infos = scene.GetInstanceInfos();

    bottom_level_as_.clear();
    top_level_as_.reset();
    bottom_level_as_.reserve(mesh_infos.size());

    for (MeshInfo const& mesh : mesh_infos)
    {
        if (mesh.triangle_count == 0)
        {
            bottom_level_as_.push_back(nullptr);
            continue;
        }

        gpu::AccelerationStructureGeometryDesc geometry = {};
        geometry.vertex_buffer = vertex_buffer;
        geometry.vertex_offset = mesh.vertex_offset * sizeof(Vertex);
        geometry.vertex_stride = sizeof(Vertex);
        geometry.vertex_count = static_cast<uint32_t>(scene.GetVertices().size() - mesh.vertex_offset);
        geometry.vertex_format = gpu::ImageFormat::kRGB32_Float;
        geometry.index_buffer = index_buffer;
        geometry.index_offset = mesh.index_offset * sizeof(uint32_t);
        geometry.index_count = mesh.triangle_count * 3u;
        geometry.opaque = true;

        gpu::AccelerationStructurePtr bottom_level = device.CreateBottomLevelAccelerationStructure({geometry});
        command_buffer.BuildBottomLevelAccelerationStructure(*bottom_level, {geometry});
        bottom_level_as_.push_back(std::move(bottom_level));
    }

    std::vector<gpu::AccelerationStructureInstanceDesc> instances;
    instances.reserve(instance_infos.size());
    for (uint32_t instance_index = 0; instance_index < instance_infos.size(); ++instance_index)
    {
        InstanceInfo const& instance_info = instance_infos[instance_index];
        if (instance_info.mesh_index >= bottom_level_as_.size())
        {
            throw std::runtime_error("HardwareRtAccelerationStructure::Build: instance mesh index is out of range");
        }

        gpu::AccelerationStructurePtr const& bottom_level = bottom_level_as_[instance_info.mesh_index];
        if (!bottom_level)
        {
            continue;
        }

        gpu::AccelerationStructureInstanceDesc instance = {};
        instance.bottom_level = bottom_level.get();
        StoreTransform3x4(instance_info.transform, instance.transform);
        instance.instance_id = instance_index;
        instance.instance_mask = 0xffu;
        instances.push_back(instance);
    }

    if (instances.empty())
    {
        return;
    }

    top_level_as_ = device.CreateTopLevelAccelerationStructure(static_cast<uint32_t>(instances.size()));
    command_buffer.BuildTopLevelAccelerationStructure(*top_level_as_, instances);
}
