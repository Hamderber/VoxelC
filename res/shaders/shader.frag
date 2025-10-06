#version 460
// https://vulkan-tutorial.com/Vertex_buffers/Vertex_input_description
// It is important to know that some types, like dvec3 64 bit vectors, use multiple slots.
// That means that the index after it must be at least 2 higher.
layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1.0);
}