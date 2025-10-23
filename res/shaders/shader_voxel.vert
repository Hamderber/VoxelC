#version 460

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

layout(push_constant) uniform PushModel {
    ivec3 chunkOriginWorld;
    uint reserved;
} pc;

layout(std430, binding = 2) readonly buffer VoxelInstances {
    // 00000000                 | 000      | 000000000000000000000
    // Position 0-4095 in chunk | Rotation | Unused                  
    uint instances[];
};

// Not all gpus support 64-bit ops so just use two 32s
// bits 0-20 = Z
// bits 21-41 = Y
// bits 42-62 = X
// bit 63 = Reserved
uint packedCoordLeft;
uint packedCoordRight;
// packed data
uint packedData;
    
uvec3 unpackLocalXYZ(uint packed12)
{
    uint x =  packed12        & 0xFu;
    uint y = (packed12 >> 4u) & 0xFu;
    uint z = (packed12 >> 8u) & 0xFu;

    return uvec3(x, y, z);
}

// 000 North
// 001 South
// 010 East
// 011 West
// 100 Up
// 101 Down
// 110 Reserved
// 111 Reserved
mat3 rotationFromCode(uint code)
{
    if (code == 0u) { // +Z
        return mat3( vec3(1,0,0), vec3(0,1,0), vec3(0,0,1) );
    }
    if (code == 1u) { // -Z (rotY 180)
        return mat3( vec3(-1,0,0), vec3(0,1,0), vec3(0,0,-1) );
    }
    if (code == 2u) { // +X (rotY +90)
        return mat3( vec3(0,0,1), vec3(0,1,0), vec3(-1,0,0) );
    }
    if (code == 3u) { // -X (rotY -90)
        return mat3( vec3(0,0,-1), vec3(0,1,0), vec3(1,0,0) );
    }
    if (code == 4u) { // +Y (rotX -90)
        return mat3( vec3(1,0,0), vec3(0,0,1), vec3(0,-1,0) );
    }
    if (code == 5u) { // -Y (rotX +90)
        return mat3( vec3(1,0,0), vec3(0,0,-1), vec3(0,1,0) );
    }
    // 6u and 7u reserved (pass identity)
    return mat3(1.0);
}


layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inColor;
layout(location=2) in vec2 inTexCoord;

layout(location=0) out vec3 fragColor;
layout(location=1) out vec2 fragTexCoord;

void main() {
    uint packed = instances[gl_InstanceIndex];
    // First 12 bits
    uint local12 = packed & 0xFFFu;
    // Last 4 bits
    uint rotCode = (packed >> 12u) & 0x7u;
    // uint material  =  packed >> 15u; nyi

    uvec3 lxyz = unpackLocalXYZ(local12);

    vec3 voxelBase = vec3(lxyz);
    vec3 rotatedPos = rotationFromCode(rotCode) * inPosition;
    vec3 worldPos = vec3(pc.chunkOriginWorld) + voxelBase + rotatedPos;

    gl_Position = ubo.proj * ubo.view * vec4(worldPos, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}