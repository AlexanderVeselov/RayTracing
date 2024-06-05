@echo off
IF NOT EXIST assets\Bistro\Exterior\exterior.obj (
    echo Downloading Bistro scene files
    pushd assets
    call download_bistro.bat
    popd
)

mkdir build
pushd build
cmake ..
cmake --build . --config Release
popd

.\build\src\Release\RayTracingApp.exe --scene assets\Bistro\Exterior\exterior.obj --flip_yz 1 --scale 0.01
