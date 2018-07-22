#ifndef IMAGE_LOADER_HPP
#define IMAGE_LOADER_HPP

class Image
{
public:
    int width, height;
    // each pixel takes 4 32-bit floats, each component can be of any value...
    float* colors;
};

class HDRLoader
{
public:
    static bool Load(const char *fileName, Image &res);
};

#endif // IMAGE_LOADER_HPP
