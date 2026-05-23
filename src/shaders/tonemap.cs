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

IMAGE_FORMAT("rgba8")
RWTexture2D<float4> g_Output : register(u0);

IMAGE_FORMAT("rgba16f")
RWTexture2D<float4> g_InputColor : register(u1);

float3 FilmicCurve(float3 x)
{
    // Hable / Uncharted 2 filmic curve
    const float A = 0.15;
    const float B = 0.50;
    const float C = 0.10;
    const float D = 0.20;
    const float E = 0.02;
    const float F = 0.30;

    return ((x * (A * x + C * B) + D * E) /
            (x * (A * x + B) + D * F)) - E / F;
}

float3 LinearToSRGB(float3 x)
{
    x = max(x, 0.0);
    return select(x <= 0.0031308, x * 12.92, 1.055 * pow(x, 1.0 / 2.4) - 0.055);
}

float3 TonemapFilmic(float3 hdrColor, float exposure)
{
    float3 color = hdrColor * exposure;

    // Apply filmic curve
    color = FilmicCurve(color);

    // Normalize white point
    const float W = 11.2;
    float whiteScale = 1.0 / FilmicCurve(W).r;
    color *= whiteScale;

    return color;
}

[numthreads(8, 8, 1)]
void main(uint3 dispatch_thread_id: SV_DispatchThreadID)
{
    uint width;
    uint height;
    g_Output.GetDimensions(width, height);
    if (dispatch_thread_id.x >= width || dispatch_thread_id.y >= height)
    {
        return;
    }

    uint2 pixel_coord = dispatch_thread_id.xy;
    float3 color = g_InputColor[pixel_coord].xyz;
    color = TonemapFilmic(color, 1.0f);
    color = LinearToSRGB(color);
    g_Output[pixel_coord] = float4(color, 1.0f);
}
