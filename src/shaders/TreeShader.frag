#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;


layout(binding = 0) uniform UniformBufferObject {
    mat4 mvpMat;
    mat4 mMat;
    mat4 nMat;
    vec4 color;
} ubo;

layout(binding = 1) uniform sampler2D texSampler;

layout(binding = 2) uniform GlobalUniformBufferObject {
    vec3 lightDir[5];
    vec3 lightPos[5];
    vec4 lightColor[5];
    float cosIn;
    float cosOut;
    vec3 eyePos;
    vec4 eyeDir;
    vec4 lightOn;
    int lightType;
} gubo;


void main() {
    
    vec3 Albedo = texture(texSampler, fragTexCoord).rgb;
    
    // Normalize normal and light direction
    vec3 N = normalize(fragNormal);
    vec3 L = normalize(gubo.lightDir[0]);

    // Dot product for lighting
    float intensity = max(dot(N, L), 0.0);
    //Specular = vec3(pow(max(dot(EyeDir, -reflect(LD, Norm)),0.0f), 160.0f));
    
    // Quantize intensity into discrete steps
    float levels = 3.0; // Number of shading levels
    intensity = floor(intensity * levels) / levels;

    // Calculate final color
    vec3 color = Albedo * gubo.lightColor[0].rgb * intensity;
    outColor = vec4(color, 1.0);
}
