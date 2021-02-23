#!/bin/bash

source ./platform_detect.sh

if [[ "$platform_suffix" == "win" ]]; then
    mkdir tools
    cd tools
    git clone https://github.com/microsoft/vcpkg
    ./vcpkg/bootstrap-vcpkg.bat


    vcpkg/vcpkg install freetype
    vcpkg/vcpkg install freetype:x64-windows
    vcpkg/vcpkg integrate install
fi