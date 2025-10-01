@echo off
echo Compiling shaders...

glslc -mfmt=c res\shaders\shader.frag -o res\shaders\frag.spv
glslc -mfmt=c res\shaders\shader.vert -o res\shaders\vert.spv

echo Complete.