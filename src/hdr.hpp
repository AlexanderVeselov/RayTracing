#ifndef HDR_HPP
#define HDR_HPP

/***********************************************************************************
Created:	17:9:2002
FileName: 	hdrloader.h
Author:		Igor Kravtchenko
Info:		Load HDR image and convert to a set of float32 RGB triplet.
from http://www.flipcode.com/archives/HDR_Image_Reader.shtml
************************************************************************************/

class HDRImage {
public:
    int width, height;
    // each pixel takes 4 32-bit floats, each component can be of any value...
    float* colors;
};

class HDRLoader {
public:
    static bool Load(const char *fileName, HDRImage &res);
};

#endif // HDR_HPP
