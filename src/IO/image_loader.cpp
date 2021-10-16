/*****************************************************************************
 MIT License

 Copyright(c) 2021 Alexander Veselov

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

#include "image_loader.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <cassert>

bool LoadSTB(const char* filename, Image& result)
{
    int width;
    int height;
    int num_channels;
    unsigned char* data = stbi_load(filename, &width, &height, &num_channels, 0);
    if (!data)
    {
        return false;
    }

    std::uint32_t* uint32_data = (std::uint32_t*)data;

    result.width = width;
    result.height = height;
    result.data.resize(width * height);

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            int input_base = (y * width + x) * num_channels;
            std::uint32_t value = data[input_base];
            result.data[y * width + x] = value;
        }
    }

    stbi_image_free(data);
    return true;
}
