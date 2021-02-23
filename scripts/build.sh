#!/bin/bash


source ./platform_detect.sh

cd ../bin
mkdir minimal 2> /dev/null | true
mkdir minimal32 2> /dev/null | true
mkdir openmp 2> /dev/null | true
mkdir openmp32 2> /dev/null | true
mkdir complete 2> /dev/null | true


if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    platform_suffix="linux"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    platform_suffix="mac"
elif [[ "$OSTYPE" == "cygwin" ]]; then
    platform_suffix="win"
elif [[ "$OSTYPE" == "msys" ]]; then
    platform_suffix="win"
elif [[ "$OSTYPE" == "win32" ]]; then
    platform_suffix="win"
elif [[ "$OSTYPE" == "freebsd"* ]]; then
    platform_suffix="freebsd"
else
    echo "Unknown operating system: $OSTYPE"
    exit 1
fi

version=$(git describe --tags)
executable_suffix=""


use_32_bit=false

case $platform_suffix in
    "win")
        use_32_bit=true
        executable_suffix=".exe"
        ;;
esac

Build() {
    Arch="$1"
    Configuration="$2"
    
    OUTPUT_NAME="msdf-${Configuration}-${version}-${platform_suffix}-${Arch}"
    BUILD_DIR_PREFIX="."
    
    case $platform_suffix in
        "win") 
            case $Arch in
                "x64") GeneratorArch="-A x64" ;;
                "x86") GeneratorArch="-A Win32" ;;
            esac
            BUILD_DIR_PREFIX="Release"
            Generator="Visual Studio 16" ;;
        "linux")
            case $Arch in
                "x64") GeneratorArch="-m 64" ;;
                "x86") GeneratorArch="-m 32" ;;
            esac
            Generator="Unix Makefiles" ;;
    esac
    case $Configuration in
        "minimal") PARAMS="" ;;
        "openmp") PARAMS="-DMSDFGEN_USE_OPENMP=ON" ;;
    esac

    cmake -DCMAKE_BUILD_TYPE=Release -G "$Generator" $GeneratorArch ../.. -DMSDFGEN_USE_CPP11=ON -DMSDFGEN_BUILD_MSDFGEN_STANDALONE=ON $PARAMS
    cmake --build . --config Release -j 4
    mkdir ../complete/$OUTPUT_NAME/ 2> /dev/null | true
    cp $BUILD_DIR_PREFIX/msdfgen$executable_suffix ../complete/$OUTPUT_NAME/
}

#Minimal builds

cd minimal
Build "x64" "minimal"
cd ..

if [ "$use_32_bit" = true ] ; then
    cd minimal32
    Build "x86" "minimal"
    cd ..
fi

# OpenMP builds

cd openmp
Build "x64" "openmp"
cd ..

if [ "$use_32_bit" = true ] ; then
    cd openmp32
    Build "x86" "openmp"
    cd ..
fi