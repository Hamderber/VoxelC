@echo off
echo Compiling shaders...

glslc -mfmt=c res\shaders\shader.frag -o res\shaders\shader.frag.spv
python ConvertShaders.py res\shaders\shader.frag.spv
glslc -mfmt=c res\shaders\shader.vert -o res\shaders\shader.vert.spv
python ConvertShaders.py res\shaders\shader.vert.spv

echo Complete.