#version 460

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) flat in int faceID;

const float shadeLUT[6] = float[](1.0, 0.5, 0.8, 0.8, 0.6, 0.6);

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(texSampler, fragTexCoord) * shadeLUT[faceID];
}