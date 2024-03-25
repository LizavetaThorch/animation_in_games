@echo off
set BUILD_TYPE=%1


cd sources

cmake -G Ninja -DBUILD_TYPE=%BUILD_TYPE% -B ../Builds/%BUILD_TYPE%


cd ../Builds/%BUILD_TYPE%

ninja

set binPath=..\..
IF not exist %binPath% (mkdir %binPath%)

IF not exist %binPath%\assimp.dll (copy ..\assimp.dll %binPath%)

cd ../..