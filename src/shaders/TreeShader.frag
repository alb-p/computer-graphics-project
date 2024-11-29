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



vec3 point_light_dir(vec3 pos, int i) {
    // Point light - direction vector
    // Position of the light in <gubo.lightPos[i]>
    return normalize(gubo.lightPos[i]-pos);
}

vec3 point_light_color(vec3 pos, int i) {
    // Point light - color
    // Color of the light in <gubo.lightColor[i].rgb>
    // Scaling factor g in <gubo.lightColor[i].a>
    // Decay power beta: constant and fixed to 2.0
    // Position of the light in <gubo.lightPos[i]>
 
    vec3 lightColor = gubo.lightColor[i].rgb;
    float lightScalingFactor = gubo.lightColor[i].a;
    vec3 lightPosition = gubo.lightPos[i];
    float beta = 2.0; // Decay power
        
    // Calculate the vector from the light to the fragment
    vec3 toLight = lightPosition - pos;
    float distance = length(toLight);
        
        
    vec3 finalColor = pow((gubo.lightColor[i].a/distance),beta)*gubo.lightColor[i].rgb;
        
    return finalColor;
}


void main() {
    
    vec3 Albedo = texture(texSampler, fragTexCoord).rgb;
    vec3 N = normalize(fragNormal);
    vec3 EyeDir = normalize(gubo.eyePos - fragPos);
    
    
    vec3 LD;    // light direction
    vec3 LC;    // light color
    
    vec3 RendEqSol = vec3(0);
    
    float levels = 3.0; // Number of shading levels
    
    
    
    if (gubo.lightType == 0) {
        vec3 LD = normalize(gubo.lightDir[0]);
        vec3 LC = normalize(gubo.lightColor[0].rgb);
        float intensity = max(dot(N, LD), 0.0);
        intensity = floor(intensity * levels) / levels;
        RendEqSol += Albedo * gubo.lightColor[0].rgb * intensity  * gubo.lightOn.y;
    }
    
    else {
        // Second light
        LD = point_light_dir(fragPos, 1);
        LC = point_light_color(fragPos, 1);
        float intensity = max(dot(N, LD), 0.0);
        intensity = floor(intensity * levels) / levels;
        RendEqSol += Albedo * gubo.lightColor[0].rgb * intensity  * gubo.lightOn.x;
        
        // Third light
        LD = point_light_dir(fragPos, 2);
        LC = point_light_color(fragPos, 2);
        intensity = max(dot(N, LD), 0.0);
        intensity = floor(intensity * levels) / levels;
        RendEqSol += Albedo * gubo.lightColor[0].rgb * intensity        * gubo.lightOn.x;
        
        // Fourth light
        LD = point_light_dir(fragPos, 3);
        LC = point_light_color(fragPos, 3);
        intensity = max(dot(N, LD), 0.0);
        intensity = floor(intensity * levels) / levels;
        RendEqSol += Albedo * gubo.lightColor[0].rgb * intensity         * gubo.lightOn.x;
        
        // Fift light
        LD = point_light_dir(fragPos, 4);
        LC = point_light_color(fragPos, 4);
        intensity = max(dot(N, LD), 0.0);
        intensity = floor(intensity * levels) / levels;
        RendEqSol += Albedo * gubo.lightColor[0].rgb * intensity      * gubo.lightOn.x;
    }
    
    // Dot product for lighting
    //float intensity = max(dot(N, LD), 0.0);
    //Specular = vec3(pow(max(dot(EyeDir, -reflect(LD, Norm)),0.0f), 160.0f));
    
    // Quantize intensity into discrete steps
    
    //intensity = floor(intensity * levels) / levels;

    // Calculate final color
    //vec3 color = Albedo * gubo.lightColor[0].rgb * intensity;
    outColor = vec4(RendEqSol, 1.0);
}
