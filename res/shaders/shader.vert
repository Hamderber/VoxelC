#version 460

layout(binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 proj;
} cam;

layout(push_constant) uniform PushModel {
    // per-draw (chunks), identity for models
    mat4 model;
} pc;

layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inColor;
layout(location=2) in vec2 inTexCoord;

layout(location=0) out vec3 fragColor;
layout(location=1) out vec2 fragTexCoord;

void main() {
    gl_Position = cam.proj * cam.view * pc.model * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}