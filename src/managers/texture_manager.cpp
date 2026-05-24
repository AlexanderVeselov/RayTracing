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

#include "texture_manager.hpp"

#include "gpu_command_buffer.hpp"
#include "gpu_device.hpp"
#include "gpu_image.hpp"
#include "gpu_queue.hpp"
#include "loaders/image_loader.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace
{
std::string NormalizeTexturePath(std::filesystem::path const& path)
{
    std::filesystem::path normalized_path = std::filesystem::absolute(path).lexically_normal();
    return normalized_path.generic_string();
}

std::string LowercaseExtension(std::filesystem::path const& path)
{
    std::string extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return extension;
}
}  // namespace

TextureManager::TextureManager(gpu::Device& device)
    : device_(device), upload_queue_(device.GetQueue(gpu::QueueType::kGraphics))
{
}

uint32_t TextureManager::LoadTexture(std::filesystem::path const& path, bool srgb)
{
    std::string normalized_path = NormalizeTexturePath(path);
    auto loaded_texture = loaded_textures_.find(normalized_path);
    if (loaded_texture != loaded_textures_.end())
    {
        return loaded_texture->second;
    }

    if (textures_.size() >= kMaxTextureCount)
    {
        throw std::runtime_error("TextureManager::LoadTexture: too many textures");
    }

    Texture texture = {};
    gpu::ImageFormat format = gpu::ImageFormat::kUnknown;
    std::string extension = LowercaseExtension(normalized_path);
    if (extension == ".hdr")
    {
        if (!LoadHDR(normalized_path.c_str(), texture.width, texture.height, texture.cpu_data))
        {
            throw std::runtime_error("TextureManager::LoadTexture: failed to load HDR texture " + normalized_path);
        }
        format = gpu::ImageFormat::kRGBA32_Float;
    }
    else if (extension == ".jpg" || extension == ".jpeg" || extension == ".png" || extension == ".tga"
        || extension == ".bmp")
    {
        if (!LoadSTB(normalized_path.c_str(), texture.width, texture.height, texture.cpu_data))
        {
            throw std::runtime_error("TextureManager::LoadTexture: failed to load texture " + normalized_path);
        }
        format = srgb ? gpu::ImageFormat::kRGBA8_SRGB : gpu::ImageFormat::kRGBA8_UNorm;
    }
    else
    {
        throw std::runtime_error("TextureManager::LoadTexture: unsupported texture extension " + normalized_path);
    }

    uint32_t texture_index = static_cast<uint32_t>(textures_.size());
    texture.path = normalized_path;
    texture.format = format;
    textures_.push_back(std::move(texture));
    loaded_textures_.emplace(normalized_path, texture_index);

    return texture_index;
}

void TextureManager::UploadPendingTextures()
{
    gpu::CommandBufferPtr command_buffer = upload_queue_.CreateCommandBuffer();
    bool has_pending_textures = false;

    for (Texture& texture : textures_)
    {
        if (texture.uploaded)
        {
            continue;
        }

        texture.gpu_image = CreateGpuImage(texture, *command_buffer);
        texture.cpu_data.clear();
        texture.cpu_data.shrink_to_fit();
        texture.uploaded = true;
        has_pending_textures = true;
    }

    if (!has_pending_textures)
    {
        return;
    }

    upload_queue_.Submit(std::move(command_buffer));
    upload_queue_.WaitIdle();

    gpu_images_.clear();
    gpu_images_.reserve(textures_.size());
    for (Texture const& texture : textures_)
    {
        gpu_images_.push_back(texture.gpu_image);
    }
}

Texture const& TextureManager::GetTexture(uint32_t texture_index) const
{
    if (texture_index >= textures_.size())
    {
        throw std::runtime_error("TextureManager::GetTexture: invalid texture index");
    }

    return textures_[texture_index];
}

gpu::ImagePtr TextureManager::CreateGpuImage(Texture const& texture, gpu::CommandBuffer& command_buffer)
{
    gpu::ImagePtr image = device_.CreateImage(texture.width, texture.height, texture.format,
        gpu::ImageFlags::kShaderResource);

    if (!texture.cpu_data.empty())
    {
        command_buffer.TransitionBarrier(image, gpu::ImageLayout::kUndefined, gpu::ImageLayout::kCopyDst);
        command_buffer.UploadImage(image, texture.cpu_data.data(), texture.cpu_data.size() * sizeof(uint32_t));
        command_buffer.TransitionBarrier(image, gpu::ImageLayout::kCopyDst, gpu::ImageLayout::kShaderRead);
    }
    else
    {
        command_buffer.TransitionBarrier(image, gpu::ImageLayout::kUndefined, gpu::ImageLayout::kShaderRead);
    }

    return image;
}
