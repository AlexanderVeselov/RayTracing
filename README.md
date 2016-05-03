# Realtime Ray Tracing engine based on OpenCL

### Pre-Requisites
#### [GLFW](http://www.glfw.org/)

Linux installation
```sh
$ sudo apt-get install cmake xorg-dev libglu1-mesa-dev
$ wget https://github.com/glfw/glfw/releases/download/3.1.2/glfw-3.1.2.zip
$ unzip glfw-3.1.2.zip 
$ cd glfw-3.1.2/
$ cmake -G "Unix Makefiles"
$ make
$ sudo make install
```
#### OpenCL SDK
* [NVIDIA CUDA 7.5 SDK](https://developer.nvidia.com/cuda-downloads) - for NVIDIA GPU
* [AMD OpenCL™ Accelerated Parallel Processing](http://developer.amd.com/tools-and-sdks/opencl-zone/amd-accelerated-parallel-processing-app-sdk/) - for AMD GPU/CPU
* [Intel® SDK for OpenCL™ Applications](https://software.intel.com/en-us/intel-opencl)  - for Intel CPU/GPU
