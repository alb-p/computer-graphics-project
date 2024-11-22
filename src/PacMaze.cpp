#define JSON_DIAGNOSTICS 1
#include "modules/Starter.hpp"
#include <string>
#include <iostream>
#include <format>
#include "modules/edges_output.cpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/string_cast.hpp"


/**/
bool doSegmentsIntersect(glm::vec3 P1, glm::vec3 P2, glm::vec3 Q1, glm::vec3 Q2) {
    glm::vec3 r = P2 - P1;
    glm::vec3 s = Q2 - Q1;

    float rxs = r.x * s.z - r.z * s.x;
    glm::vec3 PQ = Q1 - P1;
    float PQxr = PQ.x * r.z - PQ.z * r.x;
    float PQxs = PQ.x * s.z - PQ.z * s.x;

    // Parallel or collinear
    if (rxs == 0) return false;

    float t = PQxs / rxs;
    float u = PQxr / rxs;

    return (0 <= t && t <= 1 && 0 <= u && u <= 1);
}

enum GameState {
    notStarted,
    playing,
    ended
};
template<typename ... Args> std::string string_format(const std::string& format, Args ... args){
    int size_s = std::snprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
    if (size_s <= 0) { throw std::runtime_error("Error during formatting."); }
    auto size = static_cast<size_t>(size_s);
    std::unique_ptr<char[]> buf(new char[size]);
    std::snprintf(buf.get(), size, format.c_str(), args ...);
    return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}




///Vertex
struct VertexOverlay {
    glm::vec2 pos;
    glm::vec2 UV;
};


struct LightVertex {
    glm::vec3 pos;
    glm::vec2 UV;
};

struct DunVertex {
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec2 UV;
    
};


struct Vertex {
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec2 UV;
    glm::vec4 tan;
    Vertex(glm::vec3 &pos, glm::vec3 &norm, glm::vec2 &UV, glm::vec4 &tan): pos(pos), norm(norm), UV(UV), tan(tan){};
};






///UniformBufferObjects
struct OverlayUniformBlock {
    alignas(4) float visible;
};


struct LightUniformBufferObject {
    alignas(16) glm::mat4 mvpMat;
};

#define OBDUN 5
struct DunUniformBufferObject {
    alignas(16) glm::mat4 mvpMat[OBDUN];
    alignas(16) glm::mat4 mMat[OBDUN];
    alignas(16) glm::mat4 nMat[OBDUN];
};


struct UniformBufferObject {
    alignas(16) glm::mat4 mvpMat;
    alignas(16) glm::mat4 mMat;
    alignas(16) glm::mat4 nMat;
};


struct GlobalUniformBufferObject {
    struct {
    alignas(16) glm::vec3 v;
    } lightDir[5];
    struct {
    alignas(16) glm::vec3 v;
    } lightPos[5];
    alignas(16) glm::vec4 lightColor[5];
    alignas(4) float cosIn;
    alignas(4) float cosOut;
    alignas(16) glm::vec3 eyePos;
    alignas(16) glm::vec4 eyeDir;
    alignas(16) glm::vec4 lightOn;
    alignas(4) bool lightType = 0;
};
///





struct CollectibleItem {
    glm::vec3 position;
    bool isCollected;
    std::string name;
    CollectibleItem(glm::vec3 position,bool isCollected, std::string name):position(position),isCollected(isCollected),name(name){};
};
bool CheckCollision(const glm::vec3& playerPos, glm::vec3 itemPos, float radius){
    float distance = glm::length(playerPos - itemPos);
    return (distance < radius); // Collision if within radius
}


std::vector<unsigned char> serializeVertices(const std::vector<VertexOverlay>& vertices) {
    std::vector<unsigned char> buffer;
    buffer.resize(vertices.size() * sizeof(VertexOverlay));
    memcpy(buffer.data(), vertices.data(), buffer.size());
    return buffer;
}




#include "modules/Scene.hpp"
#include <glm/glm.hpp>
#include <iostream>
#include <limits>

// Epsilon for floating-point comparisons
const float EPSILON = 1e-6f;



class CGproj;
void GameLogic(CGproj *A, float Ar, glm::mat4 &ViewPrj, glm::mat4 &World);
class CGproj : public BaseProject {
protected:
    GameState game_state = notStarted;
    
    DescriptorSetLayout DSL, DSLOverlay, DSLLight, DSLDun;
    
    VertexDescriptor VD, VOverlay, VDLight, VDDun;
    
    Pipeline P, POverlay, PLight, PDun;
    
    Model MText[3], MHUD[4], MLight, MEnemy;
    
    DescriptorSet DSText[3], DSHUD[4], DSLight, DSDun;
    
    Texture TText[3], THUD[4], TLight, TEnemy;
    
    OverlayUniformBlock uboText[3], uboHUD[4];
    
    Scene SC;
    
    glm::vec3 **deltaP;
    float *deltaA;
    float *usePitch;
    int currScene = 0;
    float Ar;
    float doorAngle = 0.0f;
    glm::vec3 Pos;
    glm::vec3 oldPos;
    glm::vec3 ghostPosition_old;
    float Yaw;
    glm::vec3 InitialPos;
    int score = 0;
    bool gameStarted = false;
    bool gameWon = false;
    bool lightDirectional = false;

    
    std::vector<std::string> landscape =  {"pavimento","hint1","hint2", "maze", "sky", "portal1","portal2","portal3", "door", "grave1", "grave2","grave3","grave4","grave5", "ghost"};
        glm::vec3 scaleFactorSkyToHide = glm::vec3(1.0,1.0,1.0);
        glm::vec3 scaleFactorFloorToHide = glm::vec3(1.0,1.0,1.0);
        std::vector<std::string> subject = {"c1"};
        glm::vec3 item1Position =  glm::vec3(-34.7,0.0,-32.3);
        glm::vec3 item2Position =  glm::vec3(-17.75, 0.0, -0.93);
        glm::vec3 item3Position =  glm::vec3(21.65, 0.0, 33.64);
        glm::vec3 trap1Position =  glm::vec3(-28.5,0.0,-16.4655);
        glm::vec3 trap2Position =  glm::vec3(-38.8831,0.0,10.5052);
        glm::vec3 trap3Position =  glm::vec3(-13.0604,0.0, -25.3974);
        glm::vec3 trap4Position =  glm::vec3(10.9513, 0.0, 31.8355);
        glm::vec3 trap5Position =  glm::vec3(24.4103,0.0,-34.1741);
        glm::vec3 portalPosition = glm::vec3(30.0, 0.0, -21.54);
        glm::vec3 hint1Position = glm::vec3(-3.83, 0.0, 5.48);
        glm::vec3 hint2Position = glm::vec3(23.675, 0.0,3.19);
        glm::vec3 doorPosition =  glm::vec3(-13.38, 0.0, 38.0);
        glm::vec3 ghostPosition =  glm::vec3(-5, 0.0, -10.0);


    CollectibleItem object1 = CollectibleItem(item1Position,false,"objectToCollect");
    CollectibleItem object2 = CollectibleItem(item2Position,false,"objectToCollect2");
    CollectibleItem object3 = CollectibleItem(item3Position,false,"objectToCollect3");
        
   

    void setWindowParameters() {
        windowWidth = 1280;
        windowHeight = 720;
        windowTitle = "PAC-MAZE";
        windowResizable = GLFW_TRUE;
        initialBackgroundColor = {36/255,30/255,43/255, 1.0f};
        uniformBlocksInPool = 25 * 2 + 2;
        texturesInPool = 29 + 1;
        setsInPool = 29 + 1;
        Ar = 4.0f / 3.0f;
    }
    
    void onWindowResize(int w, int h) {
        Ar = (float)w / (float)h;
    }
    
    void localInit() {
        glm::vec2 anchor = glm::vec2(-0.7f);
        float h = 0.45;
        float w = 0.55;
        DSL.init(this, {
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS},
            {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
            {2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS}
        });
        DSLOverlay.init(this, {
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
                {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
            });
        
        DSLLight.init(this, {
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
            {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
            });
        
        DSLDun.init(this, {
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS},
            {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
            {2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS}
        });
        
        VD.init(this, {
            {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}
        }, {
            {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos),
                sizeof(glm::vec3), POSITION},
            {0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, UV),
                sizeof(glm::vec2), UV},
            {0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, norm),
                sizeof(glm::vec3), NORMAL}
        });
        VOverlay.init(this, {
            {0, sizeof(VertexOverlay), VK_VERTEX_INPUT_RATE_VERTEX}
            }, {
                  {0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(VertexOverlay, pos),
                         sizeof(glm::vec2), OTHER},
                  {0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(VertexOverlay, UV),
                         sizeof(glm::vec2), UV}
            });
        
        VDLight.init(this, {
                  {0, sizeof(LightVertex), VK_VERTEX_INPUT_RATE_VERTEX}
                }, {
                    {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(LightVertex, pos),
                        sizeof(glm::vec3), POSITION},
                    {0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(LightVertex, UV),
                        sizeof(glm::vec2), UV}
                });
        
        
        VDDun.init(this, {
                  {0, sizeof(LightVertex), VK_VERTEX_INPUT_RATE_VERTEX}
                }, {
                    {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(DunVertex, pos),
                        sizeof(glm::vec3), POSITION},
                    {0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(DunVertex, UV),
                        sizeof(glm::vec2), UV},
                    {0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(DunVertex, norm),
                        sizeof(glm::vec3), NORMAL}
                });
        
        
        
        P.init(this, &VD, "shaders/PhongVert.spv", "shaders/PhongFrag.spv", {&DSL});
        P.setAdvancedFeatures(VK_COMPARE_OP_LESS_OR_EQUAL, VK_POLYGON_MODE_FILL,
                              VK_CULL_MODE_NONE, false);
        
        
        PDun.init(this, &VDDun,  "shaders/PhongVert.spv", "shaders/PhongFrag.spv", {&DSLDun});
        P.setAdvancedFeatures(VK_COMPARE_OP_LESS_OR_EQUAL, VK_POLYGON_MODE_FILL,
                              VK_CULL_MODE_NONE, false);
        
        
        POverlay.init(this, &VOverlay, "shaders/OverlayVert.spv", "shaders/OverlayFrag.spv", { &DSLOverlay });
        POverlay.setAdvancedFeatures(VK_COMPARE_OP_LESS_OR_EQUAL, VK_POLYGON_MODE_FILL,
            VK_CULL_MODE_NONE, true);
        
        
        
        
        PLight.init(this, &VDLight, "shaders/LightVert.spv", "shaders/LightFrag.spv", {&DSLLight});
        PLight.setAdvancedFeatures(VK_COMPARE_OP_LESS_OR_EQUAL, VK_POLYGON_MODE_FILL,
            VK_CULL_MODE_NONE, true);
        
        
        MLight.init(this, &VDLight, "models/Sphere.obj", OBJ);
        MEnemy.init(this, &VDLight, "models/barrel.002_Mesh.4450.mgcg", MGCG);
        
        
        //resize whole screens
        std::vector<VertexOverlay> vertexData;
        for (int i = 0; i<3; i++){
            float scaleFactorW = 2.5f;
            float scaleFactorH = 4.0f;
            float translationY = -0.35f;
            float translationX = -0.65f;
             vertexData = {
                {{anchor.x + translationX, anchor.y + translationY}, {0.0f, 0.0f}},
                {{anchor.x + translationX, anchor.y + h * 1.15f*scaleFactorH + translationY}, {0.0f, 1.0f}},
                {{anchor.x + w * 2*scaleFactorW + translationX, anchor.y + translationY}, {1.0f, 0.0f}},
                {{anchor.x + w * 2*scaleFactorW + translationX, anchor.y + h * 1.15f* scaleFactorH + translationY}, {1.0f, 1.0f}}
            };
            MText[i].vertices = serializeVertices(vertexData);
            MText[i].indices = { 0, 1, 2,    1, 2, 3 };
            MText[i].initMesh(this, &VOverlay);
        }
        
        //resize HUD
        anchor = glm::vec2(-0.95, -0.95);
        h = 0.25;
        w = 0.1;
        for (int i = 0; i < 4; i++)
        {
            float scaleFactor = 2.5f;
            float translationY = -0.25f;
            vertexData = {
                {{anchor.x, anchor.y + translationY}, {0.0f, 0.0f}},
                {{anchor.x, anchor.y + h * scaleFactor + translationY}, {0.0f, 1.0f}},
                {{anchor.x + w * 2*scaleFactor, anchor.y + translationY}, {1.0f, 0.0f}},
                {{anchor.x + w * 2*scaleFactor, anchor.y + h * scaleFactor + translationY}, {1.0f, 1.0f}}
            };
            MHUD[i].vertices = serializeVertices(vertexData);
            MHUD[i].indices = { 0, 1, 2,    1, 2, 3 };
            MHUD[i].initMesh(this, &VOverlay);
        }
        
        // Load Scene
        SC.init(this, &VD, DSL, P, "models/scene.json");
        for(int i = 0; i < 3; i++){
            TText[i].init(this, string_format("textures/wholeScreen%d.jpeg",i).c_str());
        }
        for (int i = 0; i < 4; i++)
        {
            THUD[i].init(this, string_format("textures/keys%d.png", i ).c_str());
        }
        
        TLight.init(this, "textures/2k_sun.jpg");
        TEnemy.init(this, "textures/barrel.002_Mesh.4450.png");
        
        Pos = glm::vec3(0.0f,0.0f,0.0f);
        InitialPos = Pos;
        Yaw = 0;
        deltaP = (glm::vec3 **)calloc(SC.InstanceCount, sizeof(glm::vec3 *));
        deltaA = (float *)calloc(SC.InstanceCount, sizeof(float));
        usePitch = (float *)calloc(SC.InstanceCount, sizeof(float));
        for(int i=0; i < SC.InstanceCount; i++) {
            deltaP[i] = new glm::vec3(SC.I[i].Wm[3]);
            deltaA[i] = 0.0f;
            usePitch[i] = 0.0f;
        }
    }
    
    void pipelinesAndDescriptorSetsInit() {
        P.create();
        POverlay.create();
        
        PLight.create();
        PDun.create();
        
        SC.pipelinesAndDescriptorSetsInit(DSL);
        
        
        DSLight.init(this, &DSLLight, {
            {0, UNIFORM, sizeof(LightUniformBufferObject), nullptr},
            {1, TEXTURE, 0, &TLight}
            });
        
        DSDun.init(this, &DSLDun, {
            {0, UNIFORM, sizeof(DunUniformBufferObject), nullptr},
            {1, TEXTURE, 0, &TEnemy}
        });
        
        for (int i = 0; i < 3; i++){
            DSText[i].init(this, &DSLOverlay, {
                {0, UNIFORM, sizeof(OverlayUniformBlock), nullptr},
                {1, TEXTURE, 0, &TText[i]}
            });
        }
        for (int i = 0; i < 4; i++)
        {
            DSHUD[i].init(this, &DSLOverlay, {
                    {0, UNIFORM, sizeof(OverlayUniformBlock), nullptr},
                    {1, TEXTURE, 0, &THUD[i]}
                });
        }
    }
    void pipelinesAndDescriptorSetsCleanup() {
        // Cleanup pipelines
        P.cleanup();
        POverlay.cleanup();
        PLight.cleanup();
        PDun.cleanup();
        SC.pipelinesAndDescriptorSetsCleanup();
        
        DSLight.cleanup();
        DSDun.cleanup();
        
        for (int i = 0; i < 3; i++){
            DSText[i].cleanup();
        }
        for (int i = 0; i < 4; i++){
            DSHUD[i].cleanup();
        }
        
    }
    void localCleanup() {
        for(int i=0; i < SC.InstanceCount; i++) {
            delete deltaP[i];
        }
        free(deltaP);
        free(deltaA);
        free(usePitch);
        DSL.cleanup();
        for (int i = 0; i < 3;i++){
            TText[i].cleanup();
            MText[i].cleanup();
        }
        for (int i = 0; i < 4; i++)
        {
            THUD[i].cleanup();
            MHUD[i].cleanup();
        }
        DSLOverlay.cleanup();
        DSLLight.cleanup();
        DSLDun.cleanup();
        
        MLight.cleanup();
        MEnemy.cleanup();
        
        TLight.cleanup();
        TEnemy.cleanup();
        
        P.destroy();
        POverlay.destroy();
        PLight.destroy();
        PDun.destroy();
        SC.localCleanup();
    }
    void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {
        P.bind(commandBuffer);
        SC.populateCommandBuffer(commandBuffer, currentImage, P);
        
        
        
        
        PLight.bind(commandBuffer);
        MLight.bind(commandBuffer);
        DSLight.bind(commandBuffer, PLight, 0, currentImage);
        vkCmdDrawIndexed(commandBuffer,
                static_cast<uint32_t>(MLight.indices.size()), 1, 0, 0, 0);
        
        
        PDun.bind(commandBuffer);
        MEnemy.bind(commandBuffer);
        DSDun.bind(commandBuffer, PLight, 0, currentImage);
        vkCmdDrawIndexed(commandBuffer,
                static_cast<uint32_t>(MEnemy.indices.size()), 5, 0, 0, 0);
        POverlay.bind(commandBuffer);
        for (int i = 0; i < 3; i++){
            MText[i].bind(commandBuffer);
            DSText[i].bind(commandBuffer, POverlay, 0, currentImage);
            vkCmdDrawIndexed(commandBuffer,
                             static_cast<uint32_t>(MText[i].indices.size()), 1, 0, 0, 0);
        }
        for (int i = 0; i < 4; i++)
        {
            MHUD[i].bind(commandBuffer);
            DSHUD[i].bind(commandBuffer, POverlay, 0, currentImage);
            vkCmdDrawIndexed(commandBuffer,
                static_cast<uint32_t>(MHUD[i].indices.size()), 1, 0, 0, 0);

        }
        for (int i = 0; i < 3; i++){
            uboText[i].visible = 0.0f;
            DSText[i].map(currentImage, &uboText[i], sizeof(uboText[i]),0);
        }
        for (int i = 0; i < 4; i++) {
            uboHUD[i].visible = 0.0f;
            DSHUD[i].map(currentImage, &uboHUD[i], sizeof(uboHUD[i]), 0);
        }
        
    }
    glm::vec3 posToCheck = glm::vec3(Pos.x, Pos.y, Pos.z);
    std::vector<CollectibleItem> collectibleItems = {object1, object2, object3};
    
    //ghost vect
    glm::vec3 gx = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 gz = glm::vec3(0.0f, 0.0f, 1.0f);



    void updateUniformBuffer(uint32_t currentImage) {
        if(glfwGetKey(window, GLFW_KEY_ESCAPE)) {
            glfwSetWindowShouldClose(window, GL_TRUE);
        }
        glm::mat4 ViewPrj;
        glm::mat4 WM;
        const float FOVy = glm::radians(45.0f);
        const float nearPlane = 0.1f;
        const float farPlane = 100.f;
        const glm::vec3 StartingPosition = glm::vec3(-12.38, 0.0, -10.0);
        float camHeight = 0.5f;
        float camDist = 1.5f;
        const float minPitch = glm::radians(-8.75f);
        const float maxPitch = glm::radians(60.0f);
        const float ROT_SPEED = glm::radians(120.0f);
        const float MOVE_SPEED = 10.0f;
        float deltaT;
        glm::vec3 m = glm::vec3(0.0f), r = glm::vec3(0.0f);
        bool fire = false;
        bool start = false;
        
        bool hideMaze = false;
        bool changeLight = false;
        bool mazeVisible = true;
        glm::vec3 subjScaleFactor = glm::vec3(1.0,1.0,1.0);
        scaleFactorSkyToHide = glm::vec3(1.0,1.0,1.0);
        scaleFactorFloorToHide = glm::vec3(1.0,1.0,1.0);
    
        getSixAxis(deltaT, m, r, fire, start, hideMaze, changeLight);
        
        static glm::vec3 Pos = StartingPosition;
        glm::mat4 ViewPrjOld = glm::mat4(1);
        static float Yaw = glm::radians(0.0f);
        static float Pitch = glm::radians(0.0f);
        static float Roll = glm::radians(0.0f);
    
        UniformBufferObject ubo{};
        GlobalUniformBufferObject gubo{};

        if(changeLight){
            lightDirectional = !lightDirectional;
        }
        if(start){
            gameStarted = true;
            game_state = playing;
            uboText[0].visible = 0.0f;
            DSText[0].map(currentImage, &uboText[0], sizeof(uboText[0]), 0);
        }
        
        glm::mat4 baseTr = glm::mat4(1.0f);
        glm::mat4 Prj = glm::perspective(FOVy, Ar, nearPlane, farPlane);
        Prj[1][1] *= -1;
        glm::vec3 target, cameraPos;
        glm::mat4 View;
        glm::vec3 ux, uy, uz;
        float lambda = 10;
        
        switch (game_state) {
            case notStarted:
                //Show homepage
                uboText[0].visible = 1.0f;
                DSText[0].map(currentImage, &uboText[0], sizeof(uboText[0]), 0);
                for (int i = 1; i < 3; i++){
                    uboText[i].visible = 0.0f;
                    DSText[i].map(currentImage, &uboText[i], sizeof(uboText[i]),0);
                }
                
                //Hide HUD
                for (int i = 0; i < 4; i++)
                {
                    uboHUD[i].visible = 0.0f;
                    DSHUD[i].map(currentImage, &uboHUD[i], sizeof(uboHUD[i]), 0);
                }
                break;
            case playing:
                //Stop showing homepage
                for (int i = 0; i < 3; i++){
                    uboText[i].visible = 0.0f;
                    DSText[i].map(currentImage, &uboText[i], sizeof(uboText[i]),0);
                }
                
                score = 0;
                if(object1.isCollected == false && !(count(landscape.begin(), landscape.end(), object1.name)>0)){
                    landscape.push_back(object1.name);
                }
                if(object2.isCollected == false && !(count(landscape.begin(), landscape.end(), object2.name)>0)){
                    landscape.push_back(object2.name);
                }
                if(object3.isCollected == false && !(count(landscape.begin(), landscape.end(), object3.name)>0)){
                    landscape.push_back(object3.name);
                }
                if(hideMaze){
                    mazeVisible = false;
                }
                
                if(object1.isCollected){
                    score++;
                }
                if(object2.isCollected){
                    score++;
                }
                if(object3.isCollected){
                    score++;
                }
                
                /*
                if(score == 3){
                gameWon = true;
                game_state = ended;
                break;
                }*/
                
                // ghost movement
                ghostPosition_old = ghostPosition;
                
                ghostPosition = ghostPosition + MOVE_SPEED/1.5f * gx * deltaT;

                ghostPosition  = ghostPosition + MOVE_SPEED/1.5f * gz * deltaT;
                // std::cout << "\nghost vect";
                // std::cout <<  gx.x << ", " << gz.z << ";\n";
                
                for(auto &coppia : vertex_pairs){
                        if(doSegmentsIntersect(ghostPosition, ghostPosition_old, coppia.second, coppia.first)){
                            gx.x = (float(sin(glfwGetTime()*100)));                            
                            gz.z = (float(cos(glfwGetTime()*100))); 
                            ghostPosition = ghostPosition_old;
                            std::cout << "Ghost Collision detected\n";
                            std::cout <<  gx.x << ", " << gz.z << ";\n\n";
                            break;
                        }
                    }
                

                //compute movement
                oldPos = Pos; //to set again in case of collision with walls
                
                ux = glm::rotate(glm::mat4(1.0f), Yaw, glm::vec3(0,1,0)) * glm::vec4(1,0,0,1);
                uy = glm::vec3(0,1,0);
                uz = glm::rotate(glm::mat4(1.0f), Yaw, glm::vec3(0,1,0)) * glm::vec4(0,0,1,1);
                Pos = Pos + MOVE_SPEED * m.x * ux * deltaT;
                Pos = Pos + MOVE_SPEED * m.y * uy * deltaT;
                Pos.y = Pos.y < 0.0f ? 0.0f : Pos.y;
                Pos = Pos + MOVE_SPEED * m.z * uz * deltaT;
                //std::cout << Pos.x << ", " << Pos.y << ", " <<  Pos.z << ";\n";
                std::cout << ghostPosition.x << ", " << ghostPosition.y << ", " <<  ghostPosition.z << ";\n";

                if(Pos.y == 0){
                    for(auto &coppia : vertex_pairs){
                        if(doSegmentsIntersect(Pos, oldPos, coppia.second, coppia.first)){
                            Pos = oldPos;
                            std::cout << "Collision detected \n\n";
                            break;
                        }
                    }
                }
                
                Yaw = Yaw - ROT_SPEED * deltaT * r.y;
                Pitch = Pitch + ROT_SPEED * deltaT * r.x;
                Pitch  =  Pitch < minPitch ? minPitch :
                (Pitch > maxPitch ? maxPitch : Pitch);
                Roll   = Roll  - ROT_SPEED * deltaT * r.z;
                Roll   = Roll < glm::radians(-175.0f) ? glm::radians(-175.0f) :
                (Roll > glm::radians( 175.0f) ? glm::radians( 175.0f) : Roll);
                
                WM = glm::translate(glm::mat4(1.0), Pos) * glm::rotate(glm::mat4(1.0f), Yaw, glm::vec3(0,1,0));
                Prj = glm::perspective(FOVy, Ar, nearPlane, farPlane);
                Prj[1][1] *= -1;
                target = Pos + glm::vec3(0.0f, camHeight, 0.0f);
                cameraPos = WM * glm::vec4(0.0f, camHeight + (camDist * sin(Pitch)), (camDist * cos(Pitch)), 1.0);
        
                if (!object1.isCollected && CheckCollision(Pos, item1Position, 2)) {
                    object1.isCollected = true;
                    //PlaySoundEffect("collect.wav");
                }
                if (!object2.isCollected && CheckCollision(Pos, item2Position, 2)) {
                    object2.isCollected = true;
                    //PlaySoundEffect("collect.wav");
                }
                if (!object3.isCollected && CheckCollision(Pos, item3Position, 2)) {
                    object3.isCollected = true;
                    //PlaySoundEffect("collect.wav");
                }
                if(CheckCollision(Pos, doorPosition, 1) /*&& score == 3*/){
                    doorAngle = 90.0f;
                } if (CheckCollision(Pos, ghostPosition, 1)){
                    gameWon = false;
                    game_state = ended;
                }
                                
                  
                                
                for(auto &coppia : vertex_pairs){
                    if(doSegmentsIntersect(Pos, cameraPos, coppia.second, coppia.first)){
                        camDist = 0.01f;
                        //camHeight = camHeight/2;
                        subjScaleFactor = glm::vec3(0.0f,0.0f,0.0f);
                        target = Pos + glm::vec3(0.0f, camHeight, 0.0f);
                        cameraPos = WM * glm::vec4(0.0f, camHeight + (camDist * sin(Pitch)), (camDist * cos(Pitch)), 1.0);
                     
                        break;
                    }
                }
                                
                    View = glm::rotate(glm::mat4(1.0f), -Roll, glm::vec3(0,0,1)) * glm::lookAt(cameraPos, target, glm::vec3(0,1,0));
                    
                   
                    
                    if(CheckCollision(Pos, portalPosition, 1)){
                        Pos = glm::vec3(-17.4279, 0, 19.2095);
                    }
                    
                    
                    if(CheckCollision(Pos, hint2Position, 1)){
                        //questa riga fa hint che nasconde il labirinto temporaneamente
                        mazeVisible = false;
                    }
                    
                    if(CheckCollision(Pos, trap1Position, 1) ||CheckCollision(Pos, trap2Position, 1) ||CheckCollision(Pos, trap3Position, 1) ||CheckCollision(Pos, trap4Position, 1) || CheckCollision(Pos, trap5Position, 1)){
                        //queste due righe per fare perdere - quindi trappola vera
                        gameWon = false;
                        game_state = ended;
                    }
                WM = glm::translate(glm::mat4(1.0), Pos) * glm::rotate(glm::mat4(1.0f), Yaw, glm::vec3(0,1,0));
                Prj = glm::perspective(FOVy, Ar, nearPlane, farPlane);
                Prj[1][1] *= -1;
                target = Pos + glm::vec3(0.0f, camHeight, 0.0f);
                cameraPos = WM * glm::vec4(0.0f, camHeight + (camDist * sin(Pitch)), (camDist * cos(Pitch)), 1.0);
                View = glm::rotate(glm::mat4(1.0f), -Roll, glm::vec3(0,0,1)) * glm::lookAt(cameraPos, target, glm::vec3(0,1,0));
                ViewPrj = Prj * View;
                if (ViewPrjOld == glm::mat4(1))
                    ViewPrjOld = ViewPrj;
                ViewPrj = ViewPrjOld * exp(-lambda * deltaT) + ViewPrj * (1 - exp(-lambda * deltaT));
                ViewPrjOld = ViewPrj;
                if(CheckCollision(Pos, hint1Position,1)){
                    //queste righe hint che fanno vedere dall'alto
                    scaleFactorSkyToHide = glm::vec3(0.0,0.0,0.0);
                    scaleFactorFloorToHide = glm::vec3(0.0,0.0,0.0);
                    subjScaleFactor = glm::vec3(3.0,3.0,3.0);
                    cameraPos.y = 100.0f;
                    glm::vec3 cameraTarget = Pos;
                    glm::vec3 upVector = glm::vec3(0.0f, 1.0f, 0.0f);
                    View = glm::lookAt(cameraPos, cameraTarget, upVector);
                    ViewPrj = Prj * View;
                }
                
                

                
                /*
                gubo.lightDir = glm::vec3(cos(glm::radians(135.0f)), sin(glm::radians(135.0f)), 0.0f);
                gubo.lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
                */
                
                
                //Buffer for point lights
                /*for (int i=0; i < NUM_POINT_LIGHTS; i++) {
                    gubo.eyePos = cameraPos;
                    gubo.pointLightPositions[i] = glm::vec3(1.0 + i*5, 10.5, -5.0 + i*5);
                    gubo.pointLightColors[i] = glm::vec4(1.0 + i*1, 5.0 + i*1.2, 5.0 + i*1.3, 3.0f + i*1);
                    gubo.pointLightRadii[i] = 7.0f;
                }
                
                gubo.pointLightPositions[0] = glm::vec3(15.0f, 8.0f, 15.0f);
                gubo.pointLightColors[0] = glm::vec4(1.0 + 53.8, 5.0, 5.0 + 1.3, 3.0f + 72.2);
                gubo.pointLightRadii[0] = 12.0f;
                
                gubo.pointLightPositions[1] = glm::vec3(0.0f, 5.0f, 0.0f);
                gubo.pointLightColors[1] = glm::vec4(1.0 + 53.8, 5.0, 5.0 + 1.3, 3.0f + 72.2);
                gubo.pointLightRadii[1] = 12.0f;
                */
                
                
                static bool debounce = false;
                static int curDebounce = 0;
                
                gubo.lightType = lightDirectional;
                /*
                if(glfwGetKey(window, GLFW_KEY_L)) {
                    if(!debounce) {
                        debounce = true;
                        curDebounce = GLFW_KEY_L;
                        
                        gubo.lightType = !gubo.lightType;
                    }
                } else {
                    if((curDebounce == GLFW_KEY_L) && debounce) {
                        debounce = false;
                        curDebounce = 0;
                    }
                }
                
                if (gubo.lightType == 1)
                    std::cout << "LightType : " << gubo.lightType;
                */
                
                
                /*
                for(int i = 0; i <5; i++) {
                    gubo.lightColor[i] = glm::vec4(1, 0, 0, 2);
                    gubo.lightDir[i].v = glm::mat4(1) * glm::vec4(0,0,1,0);
                    gubo.lightPos[i].v = glm::mat4(1) * glm::vec4(0,0,0,1);
                }
                */
                
                gubo.lightDir[0].v = glm::vec3(1, 1, 1);
                gubo.lightColor[0] = glm::vec4(1, 1, 1, 0.5);
                
                
                
                gubo.lightColor[1] = glm::vec4(0, 1, 0, 0.5);
                gubo.lightColor[2] = glm::vec4(0, 0, 1, 0.3);
                gubo.lightColor[3] = glm::vec4(1, 1, 1, 0.5);
                gubo.lightColor[4] = glm::vec4(1, 0, 0, 0.7);
                
                
                gubo.lightPos[1].v = glm::vec3(10.0f, 8.0f, 15.0f);
                gubo.lightPos[2].v = glm::vec3(15.0f, 15.0f, 15.0f);
                gubo.lightPos[3].v = glm::vec3(20.0f, 8.0f, 15.0f);
                gubo.lightPos[4].v = glm::vec3(0.0f, 8.0f, 0.0f);
                
                
                gubo.cosIn = 0.4591524628390111;
                gubo.cosOut = 0.5401793718338013;
                
                
                gubo.eyePos = cameraPos;
                gubo.lightOn = glm::vec4(1);
                
                /*
                for (int i = 0; i < NUM_POINT_LIGHTS; i++) {
                    std::cout << "Light " << i << " Position: " << glm::to_string(gubo.pointLightPositions[i]) << std::endl;
                    std::cout << "Light " << i << " Color: " << glm::to_string(gubo.pointLightColors[i]) << std::endl;
                }
                */
                
                
                // Draw the subject in the scene
                for (std::vector<std::string>::iterator it = subject.begin(); it != subject.end(); it++) {
                    int i = SC.InstanceIds[it->c_str()];
                                        
                    ubo.mMat = WM * glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0,1,0));
                    ubo.mvpMat = ViewPrj * ubo.mMat;
                    ubo.nMat = glm::inverse(glm::transpose(ubo.mMat));
                    
                    SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
                    SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);
                }
                
                // Draw the landscape
                for (std::vector<std::string>::iterator it = landscape.begin(); it != landscape.end(); it++) {
                    int i = SC.InstanceIds[it->c_str()];
                    if(*SC.I[i].id == "door"){
                        glm::mat4 translationMatrix = glm::mat4(1.0f);
                        glm::mat4 rotationMatrix =  glm::rotate(glm::mat4(1.0f), glm::radians(doorAngle), glm::vec3(0.0,1.0,0.0));
                        if(doorAngle == 90.0f){
                            float tx = -0.5237f;
                            float ty = 0.0f;
                            float tz = -0.38f;
                            translationMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(tx, ty, tz));
                        }
                        ubo.mMat = SC.I[i].Wm *rotationMatrix*  baseTr * translationMatrix;
                        ubo.mvpMat = ViewPrj * ubo.mMat;
                        ubo.nMat = glm::inverse(glm::transpose(ubo.mMat));
                        SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
                        SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);
                    }
                    else if (*SC.I[i].id == "objectToCollect") {
                        if (object1.isCollected) {
                            glm::vec3 scaleToHide(0.0f, 0.0f, 0.0f);
                            glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scaleToHide);
                            ubo.mMat = scaleMatrix;
                            ubo.mvpMat = ViewPrj * ubo.mMat;
                            ubo.nMat = glm::inverse(glm::transpose(ubo.mMat));
                            SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
                            SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);
                            //continue;
                        } else {
                            // Normal transformation for uncollected objects
                            //Pos +=   uy *  (float(sin(glfwGetTime()*20))/3) * deltaT;
                            
                            glm::vec3 floatingY = glm::vec3(1.0f,(glm::abs(2+float(sin(glfwGetTime()*100))) * deltaT),1.0f);
                            glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), floatingY);
                            glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(70.0f), glm::vec3(1.0f, 0.0f, 0.0f));

                            float angle = glfwGetTime() * glm::radians(90.0f); // Rotate 90 degrees per second (you can adjust the speed)
                            glm::mat4 continuousRotation = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f)); // Around y-axis
                            
                            ubo.mMat = SC.I[i].Wm * translationMatrix * continuousRotation * rotationMatrix * baseTr;
                            ubo.mvpMat = ViewPrj * ubo.mMat;
                            ubo.nMat = glm::inverse(glm::transpose(ubo.mMat));
                            
                            SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
                            SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);
                        }
                    }
                    else if(*SC.I[i].id == "objectToCollect2") {
                        if (object2.isCollected) {
                        glm::vec3 scaleToHide(0.0f, 0.0f, 0.0f);
                        glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scaleToHide);
                        ubo.mMat = scaleMatrix;
                        ubo.mvpMat = ViewPrj * ubo.mMat;
                        ubo.nMat = glm::inverse(glm::transpose(ubo.mMat));
                        SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
                        SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);
                        //continue;
                        }
                        else {
                            // Normal transformation for uncollected objects
                            glm::vec3 floatingY = glm::vec3(1.0f,(glm::abs(2+float(sin(glfwGetTime()*100))) * deltaT),1.0f);
                            glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), floatingY);
                            glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(70.0f), glm::vec3(1.0f, 0.0f, 0.0f));

                            float angle = glfwGetTime() * glm::radians(90.0f); // Rotate 90 degrees per second (you can adjust the speed)
                            glm::mat4 continuousRotation = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f)); // Around y-axis
                            
                            ubo.mMat = SC.I[i].Wm * translationMatrix * continuousRotation * rotationMatrix * baseTr;                    ubo.mvpMat = ViewPrj * ubo.mMat;
                            ubo.nMat = glm::inverse(glm::transpose(ubo.mMat));
                            
                            SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
                            SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);
                        }
                    }
                    else if (*SC.I[i].id == "objectToCollect3") {
                        if (object3.isCollected) {
                            glm::vec3 scaleToHide(0.0f, 0.0f, 0.0f);
                            glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scaleToHide);
                            ubo.mMat = scaleMatrix;
                            ubo.mvpMat = ViewPrj * ubo.mMat;
                            ubo.nMat = glm::inverse(glm::transpose(ubo.mMat));
                            SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
                            SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);
                            //continue;
                        }
                        else {
                            // Normal transformation for uncollected objects
                            glm::vec3 floatingY = glm::vec3(1.0f,(glm::abs(10+float(sin(glfwGetTime()*100))) * deltaT),1.0f);
                            glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), floatingY);
                            glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(70.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                            float angle = glfwGetTime() * glm::radians(90.0f); // Rotate 90 degrees per second (you can adjust the speed)
                            glm::mat4 continuousRotation = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f)); // Around y-axis
                            
                            ubo.mMat = SC.I[i].Wm * translationMatrix * continuousRotation * rotationMatrix * baseTr;
                            ubo.mvpMat = ViewPrj * ubo.mMat;
                            ubo.nMat = glm::inverse(glm::transpose(ubo.mMat));
                            
                            SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
                            SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);
                        }
                    }
                    else if (*SC.I[i].id == "hint1" || *SC.I[i].id == "hint2"){
                        //glm::vec3 floatingY = glm::vec3(1.0f,(glm::abs(float(sin(glfwGetTime()*100))) * deltaT),1.0f);
                        //glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), floatingY);
                        glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(70.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                        float angle = glfwGetTime() * glm::radians(90.0f); // Rotate 90 degrees per second (you can adjust the speed)
                        glm::mat4 continuousRotation = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f)); // Around y-axis
                        
                        ubo.mMat = SC.I[i].Wm * /*translationMatrix */ continuousRotation * rotationMatrix * baseTr;
                        ubo.mvpMat = ViewPrj * ubo.mMat;
                        ubo.nMat = glm::inverse(glm::transpose(ubo.mMat));
                        
                        SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
                        SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);
                    }
                    else if (*SC.I[i].id == "maze"){
                        if(mazeVisible){
                            ubo.mMat = SC.I[i].Wm * baseTr;
                            ubo.mvpMat = ViewPrj * ubo.mMat;
                            ubo.nMat = glm::inverse(glm::transpose(ubo.mMat));
                            
                            SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
                            SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);
                        }
                        else{
                            glm::vec3 scaleToHide(0.0f, 0.0f, 0.0f);
                            glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scaleToHide);
                            ubo.mMat = scaleMatrix;
                            ubo.mvpMat = ViewPrj * ubo.mMat;
                            ubo.nMat = glm::inverse(glm::transpose(ubo.mMat));
                            SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
                            SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);
                        }
                    }
                    else if (*SC.I[i].id == "sky"){
                        glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scaleFactorSkyToHide);
                        ubo.mMat =  SC.I[i].Wm * scaleMatrix * baseTr;
                        ubo.mvpMat = ViewPrj * ubo.mMat;
                        ubo.nMat = glm::inverse(glm::transpose(ubo.mMat));
                        SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
                        SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);
                    }
                    else if (*SC.I[i].id == "pavimento"){
                        glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f),scaleFactorFloorToHide);
                        ubo.mMat = SC.I[i].Wm *scaleMatrix* baseTr;
                        ubo.mvpMat = ViewPrj * ubo.mMat;
                        ubo.nMat = glm::inverse(glm::transpose(ubo.mMat));
                        
                        SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
                        SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);
                    } else if ((*SC.I[i].id == "ghost")){
                        glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), ghostPosition);
                        ubo.mMat = SC.I[i].Wm *translationMatrix* baseTr;
                        ubo.mvpMat = ViewPrj * ubo.mMat;
                        ubo.nMat = glm::inverse(glm::transpose(ubo.mMat));
                        SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
                        SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);
                    }
                    else {
                        ubo.mMat = SC.I[i].Wm * baseTr;
                        ubo.mvpMat = ViewPrj * ubo.mMat;
                        ubo.nMat = glm::inverse(glm::transpose(ubo.mMat));
                        
                        SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
                        SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);
                    }
                    
                    LightUniformBufferObject lightUbo{};
                    lightUbo.mvpMat = ViewPrj * glm::translate(glm::mat4(1),glm::vec3(5.0f, 15.0f, 5.0f)) * baseTr;
                    //lightUbo.mMat = glm::mat4(1);
                    //lightUbo.nMat = glm::mat4(1);
                    DSLight.map(currentImage, &lightUbo, sizeof(lightUbo), 0);
                    
                    
                    glm::vec3 scaleToHide(0.0f, 0.0f, 0.0f);
                    glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scaleToHide);

                    DunUniformBufferObject dunUbo{};
                    for(int i = 0; i < OBDUN; i++) {
                        dunUbo.mMat[i] = glm::translate(glm::mat4(1), glm::vec3(10.0f*i, 0.0f, 0.0f)) * baseTr;
                        dunUbo.mvpMat[i] = ViewPrj * dunUbo.mMat[i];
                        dunUbo.nMat[i] = glm::inverse(glm::transpose(dunUbo.mMat[i]));
                    }
                    
                    DSDun.map(currentImage, &dunUbo, sizeof(dunUbo), 0);
                    
                    
                }
                for (int i = 0; i < 4; i++)
                {
                    uboHUD[i].visible = 0.0f;
                    DSHUD[i].map(currentImage, &uboHUD[i], sizeof(uboHUD[i]), 0);
                    if(i == score){
                        uboHUD[i].visible = 1.0f;
                        DSHUD[i].map(currentImage, &uboHUD[i], sizeof(uboHUD[i]), 0);
                    }
                }
                break;
            case ended:
                //Show homepage
                if (gameWon) {
                    uboText[2].visible = 1.0f;
                    DSText[2].map(currentImage, &uboText[2], sizeof(uboHUD[2]), 0);
            
                }else{
                    uboText[1].visible = 1.0f;
                    DSText[1].map(currentImage, &uboText[1], sizeof(uboHUD[1]), 0);
                }
                //Hide HUD
                for (int i = 0; i < 4; i++)
                {
                    uboHUD[i].visible = 0.0f;
                    DSHUD[i].map(currentImage, &uboHUD[i], sizeof(uboHUD[i]), 0);
                }
                Pos = StartingPosition;
                object1.isCollected = false;
                object2.isCollected = false;
                object3.isCollected = false;
                
                break;
            default:
                break;
        }
    }

};

int main() {
    CGproj app;
 
    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
 
    return EXIT_SUCCESS;
}
