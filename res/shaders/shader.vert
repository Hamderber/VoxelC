#version 460

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(push_constant) uniform PushModel {
    mat4 model;   // per-draw (chunks), identity for models
} pc;

layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inColor;
layout(location=2) in vec2 inTexCoord;

layout(location=0) out vec3 fragColor;
layout(location=1) out vec2 fragTexCoord;

void main() {
    // Compose model_from_UBO * model_from_push
    mat4 modelCombined = ubo.model * pc.model;
    gl_Position = ubo.proj * ubo.view * modelCombined * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}