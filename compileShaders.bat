@echo off
:: Compile, convert to .h via python, then delete the original compiled binary
:: Ends up with shader.frag and shader.frag.h etc
echo Compiling shaders...

:: Fragment shader
glslc -mfmt=c res\shaders\shader.frag -o res\shaders\shader.frag.spv
python ConvertShaders.py res\shaders\shader.frag.spv
del res\shaders\shader.frag.spv

:: Vertex shader
glslc -mfmt=c res\shaders\shader.vert -o res\shaders\shader.vert.spv
python ConvertShaders.py res\shaders\shader.vert.spv
del res\shaders\shader.vert.spv

echo Complete.