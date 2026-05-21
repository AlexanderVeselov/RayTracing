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

#include "gpu_types.hpp"
#include "shaders/shared_structures.h"

#include <cstdint>
#include <filesystem>
#include <unordered_map>
#include <vector>

namespace gpu
{
class CommandBuffer;
class Device;
class Queue;
}  // namespace gpu

struct Texture
{
    std::filesystem::path path;
    uint32_t width = 0;
    uint32_t height = 0;
    gpu::ImageFormat format = gpu::ImageFormat::kUnknown;
    std::vector<uint32_t> cpu_data;
    gpu::ImagePtr gpu_image;
    bool uploaded = false;
};

class TextureManager
{
public:
    static constexpr uint32_t kInvalidTexture = 0xFFu;
    static constexpr uint32_t kMaxTextureCount = MAX_TEXTURES;

    explicit TextureManager(gpu::Device& device);

    uint32_t LoadTexture(std::filesystem::path const& path);
    void UploadPendingTextures();

    uint32_t TextureCount() const { return static_cast<uint32_t>(gpu_images_.size()); }
    Texture const& GetTexture(uint32_t texture_index) const;
    std::vector<gpu::ImagePtr> const& GetImages() const { return gpu_images_; }

private:
    gpu::ImagePtr CreateGpuImage(Texture const& texture, gpu::CommandBuffer& command_buffer);

    gpu::Device& device_;
    gpu::Queue& upload_queue_;
    std::unordered_map<std::string, uint32_t> loaded_textures_;
    std::vector<Texture> textures_;
    std::vector<gpu::ImagePtr> gpu_images_;
};
