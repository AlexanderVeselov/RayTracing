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

#ifndef SAMPLING_H
#define SAMPLING_H

#define SAMPLE_TYPE_BXDF_LAYER 0
#define SAMPLE_TYPE_BXDF_U     1
#define SAMPLE_TYPE_BXDF_V     2
#define SAMPLE_TYPE_LIGHT      3
#define SAMPLE_TYPE_MAX        4

#define BLUE_NOISE_BUFFERS sobol_256spp_256d, scramblingTile, rankingTile

float SampleBlueNoise(int pixel_i, int pixel_j, int sampleIndex, int sampleDimension,
    __global int* sobol_256spp_256d, __global int* scramblingTile, __global int* rankingTile)
{
    // wrap arguments
    pixel_i = pixel_i & 127;
    pixel_j = pixel_j & 127;
    sampleIndex = sampleIndex & 255;
    sampleDimension = sampleDimension & 255;

    // xor index based on optimized ranking
    int rankedSampleIndex = sampleIndex ^ rankingTile[sampleDimension + (pixel_i + pixel_j * 128) * 8];

    // fetch value in sequence
    int value = sobol_256spp_256d[sampleDimension + rankedSampleIndex * 256];

    // If the dimension is optimized, xor sequence value based on optimized scrambling
    value = value ^ scramblingTile[(sampleDimension % 8) + (pixel_i + pixel_j * 128) * 8];

    // convert to float and return
    float v = (0.5f + value) / 256.0f;
    return v;
}

float SampleRandom(int pixel_i, int pixel_j, int sample_index, int bounce, int sample_type,
    __global int* sobol_256spp_256d, __global int* scramblingTile, __global int* rankingTile)
{
    int sample_dimension = bounce * SAMPLE_TYPE_MAX + sample_type;

#ifdef BLUE_NOISE_SAMPLER
    return SampleBlueNoise(pixel_i, pixel_j, sample_index, sample_dimension,
        sobol_256spp_256d, scramblingTile, rankingTile);
#else
    uint seed = WangHash(pixel_i);
    seed = WangHash(seed + WangHash(pixel_j));
    seed = WangHash(seed + WangHash(sample_index));
    seed = WangHash(seed + WangHash(sample_dimension));
    return seed * 2.3283064365386963e-10f;
#endif
}

#endif // SAMPLING_H
