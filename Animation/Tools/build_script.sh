#!/bin/bash
PROJECT=${1:-RayTracing}
BUILD_TYPE=${2:-rel}
REBUILD=${3:-no}
ECS_CODEGEN=${4:-no}
CMAKE=${5:-yes}
CWD=$(pwd)

cd ..
cd Builds/unix

if [ $REBUILD = "yes" ]
then
    rm -r -f $PROJECT/$BUILD_TYPE
fi

cd ../../Projects
if [ $CMAKE = "yes" ]
then
    export CC=/usr/bin/clang-6.0
    export CXX=/usr/bin/clang++-6.0
    cmake -G Ninja -DPROJECT=$PROJECT -DBUILD_TYPE=$BUILD_TYPE -B ../Builds/unix/$PROJECT/$BUILD_TYPE
fi

cd ../Builds/unix/$PROJECT/$BUILD_TYPE 
time -p ninja -j $(nproc)
mv $PROJECT-$BUILD_TYPE.exe ..
