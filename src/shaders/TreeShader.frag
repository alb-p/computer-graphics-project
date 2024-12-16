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
    
    
    vec3 ambientLight = vec3(0.15, 0.15, 0.15); // Weak ambient light
    RendEqSol += Albedo * ambientLight;
    
    vec3 specular = vec3(0.0); // Initialize specular contributio
    
    if (gubo.lightType == 0) {
        vec3 LD = normalize(gubo.lightDir[0]);
        vec3 LC = normalize(gubo.lightColor[0].rgb);
        float intensity = max(dot(N, LD), 0.0);
        intensity = floor(intensity * levels) / levels;
        
        // Specular reflection
        vec3 reflectedLight = reflect(-LD, N);
        float specFactor = pow(max(dot(reflectedLight, EyeDir), 0.0), 16.0); // Shininess of 32
        specular += gubo.lightColor[0].rgb * specFactor * 0.05; // Reduce glossiness
        
        RendEqSol += Albedo * gubo.lightColor[0].rgb * intensity  * gubo.lightOn.y;
        
    }
    
    else {
        for(int i=0; i < 5; i++) {
            LD = point_light_dir(fragPos, i);
            LC = point_light_color(fragPos, i);
            float intensity = max(dot(N, LD), 0.0);
            intensity = floor(intensity * levels) / levels;
            
            // Specular reflection for point light
            vec3 reflectedLight = reflect(-LD, N);
            float specFactor = pow(max(dot(reflectedLight, EyeDir), 0.0), 16.0); // Shininess of 32
            
            specular += LC * specFactor * 0.05; // Reduce glossiness

            
            RendEqSol += Albedo * LC * intensity  * gubo.lightOn.x;
        }
    }
    
    
    // Add specular highlights
    RendEqSol += specular;
    
    
    outColor = vec4(RendEqSol, 1.0);
}
