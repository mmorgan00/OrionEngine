#!/bin/bash

# Run from root directory!
mkdir -p bin/assets
mkdir -p bin/assets/shaders

echo "Compiling shaders..."

echo "assets/shaders/default.vert.glsl -> bin/assets/shaders/default.vert.spv"
$VULKAN_SDK/bin/glslc -fshader-stage=vert assets/shaders/default.vert.glsl -o bin/assets/shaders/default.vert.spv --target-spv=spv1.5 --target-env=vulkan1.2
ERRORLEVEL=$?
if [ $ERRORLEVEL -ne 0 ]
then
echo "Error:"$ERRORLEVEL && exit
fi

echo "assets/shaders/default.frag.glsl -> bin/assets/shaders/default.frag.spv"
$VULKAN_SDK/bin/glslc -fshader-stage=frag assets/shaders/default.frag.glsl -o bin/assets/shaders/default.frag.spv --target-spv=spv1.5 --target-env=vulkan1.2
ERRORLEVEL=$?
if [ $ERRORLEVEL -ne 0 ]
then
echo "Error:"$ERRORLEVEL && exit
fi

echo "Copying assets..."
echo cp -R "assets" "bin"
cp -R "assets" "bin"

spirv-val bin/assets/shaders/default.vert.spv
spirv-val bin/assets/shaders/default.frag.spv

echo "Done."
