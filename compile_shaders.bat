@echo off
:: Compile, convert to .h via python, then delete the original compiled binary
:: Ends up with shader.frag and shader.frag.h etc
echo Compiling shaders...

:: Fragment shaders
glslc -mfmt=c res\shaders\shader_fill.frag -o res\shaders\shader_fill.frag.spv
python convert_shaders.py res\shaders\shader_fill.frag.spv
del res\shaders\shader_fill.frag.spv

glslc -mfmt=c res\shaders\shader_wireframe.frag -o res\shaders\shader_wireframe.frag.spv
python convert_shaders.py res\shaders\shader_wireframe.frag.spv
del res\shaders\shader_wireframe.frag.spv

:: Vertex shader
glslc -mfmt=c res\shaders\shader.vert -o res\shaders\shader.vert.spv
python convert_shaders.py res\shaders\shader.vert.spv
del res\shaders\shader.vert.spv

echo Complete.