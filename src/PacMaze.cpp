#define JSON_DIAGNOSTICS 1
#include "modules/Starter.hpp"
#include <string>
#include <iostream>
#include <format>
#include "modules/edges_output.cpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/string_cast.hpp"
#include <cstdlib>


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

#define OBDUN 15


#include <vector>
#include <glm/glm.hpp> // For glm::vec3
#include <cstdlib>     // For rand() and srand()
#include <ctime>       // For seeding rand()

std::vector<glm::vec3> generateRandomPositions(int t, std::vector<glm::vec3> alreadyTakenPositions) {
    std::vector<glm::vec3> positions;
    positions.reserve(t); // Reserve memory for efficiency
    glm::vec3 newPos;

    // Seed the random number generator
    srand(static_cast<unsigned>(time(0)));

    for (int i = 0; i < t; i++) {
        float x = static_cast<float>(rand() % 80 - 40); // Random x in range [-45, 45]
        float z = static_cast<float>(rand() % 80 - 40); // Random z in range [-45, 45]
        float y = 0.0f; // Fixed y
        newPos = glm::vec3(x,y,z);
        
        bool isValid = true;
        for (const auto& pos : positions) {
            if (distance(newPos, pos) < 3.0f) {
                isValid = false;
                break;
            }
        }
        for(const auto& pos: alreadyTakenPositions){
            if(distance(newPos,pos)<5.0f){
                isValid = false;
                break;
            }
        }
        
        // If valid, add the position to the list
        if (isValid) {
            positions.push_back(newPos);
        }
        else{
            i--;
        }
    }
    return positions;
}


struct DunUniformBufferObject {
    alignas(16) glm::mat4 mvpMat;
    alignas(16) glm::mat4 mMat;
    alignas(16) glm::mat4 nMat;
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
    
    DescriptorSetLayout DSL, DSLOverlay, DSLDun;
    
    VertexDescriptor VD, VOverlay, VDDun;
    
    Pipeline P, POverlay, PScreens, PDun;
    
    Model MText[3], MHUD[4], MEnemy[OBDUN], Mprova;
    
    DescriptorSet DSText[3], DSHUD[4], DSDun[OBDUN];
    
    Texture TText[3], THUD[4], TEnemy[OBDUN];
    
    
    DunUniformBufferObject dunUbo[OBDUN];
    
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
    glm::vec3 trap1Position =  glm::vec3(-28.5,0.0,-16.4655);//V
    glm::vec3 trap2Position =  glm::vec3(-38.8831,0.0,10.5052); //V
    glm::vec3 trap3Position =  glm::vec3(-13.0604,0.0, -25.3974); //V
    glm::vec3 trap4Position =  glm::vec3(10.9513, 0.0, 31.8355); //V
    glm::vec3 trap5Position =  glm::vec3(24.4103,0.0,-34.1741); //V
    glm::vec3 portalPosition = glm::vec3(30.0, 0.0, -21.54);
    glm::vec3 hint1Position = glm::vec3(-3.83, 0.0, 5.48);
    glm::vec3 hint2Position = glm::vec3(23.675, 0.0,3.19);
    glm::vec3 doorPosition =  glm::vec3(-13.38, 0.0, 38.0);
    glm::vec3 ghostPosition =  glm::vec3(-5, 0.0, -10.0);
    const glm::vec3 StartingPosition = glm::vec3(-12.38, 0.0, -10.0);
    std::vector<glm::vec3> alreadyTakenPositions = {
            item1Position, item2Position, item3Position,
            trap1Position, trap2Position, trap3Position,
            trap4Position, trap5Position,
            portalPosition, hint1Position, hint2Position,
            doorPosition, ghostPosition, StartingPosition
        };
    
    std::vector<glm::vec3> treePosition = generateRandomPositions(OBDUN, alreadyTakenPositions);

    
    CollectibleItem object1 = CollectibleItem(item1Position,false,"objectToCollect");
    CollectibleItem object2 = CollectibleItem(item2Position,false,"objectToCollect2");
    CollectibleItem object3 = CollectibleItem(item3Position,false,"objectToCollect3");
    
    
    
    void setWindowParameters() {
        windowWidth = 1280;
        windowHeight = 720;
        windowTitle = "PAC-MAZE";
        windowResizable = GLFW_TRUE;
        initialBackgroundColor = {36/255,30/255,43/255, 1.0f};
        uniformBlocksInPool = 50 * 2 + 2;
        texturesInPool = 30 *2 + 1;
        setsInPool = 49 + 1;
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
        
        
        VDDun.init(this, {
            {0, sizeof(DunVertex), VK_VERTEX_INPUT_RATE_VERTEX}
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
        
        
        PDun.init(this, &VDDun,  "shaders/TreeVert.spv", "shaders/TreeFrag.spv", {&DSLDun});
        P.setAdvancedFeatures(VK_COMPARE_OP_LESS_OR_EQUAL, VK_POLYGON_MODE_FILL,
                              VK_CULL_MODE_NONE, false);
        
        
        POverlay.init(this, &VOverlay, "shaders/OverlayVert.spv", "shaders/OverlayFrag.spv", { &DSLOverlay });
        POverlay.setAdvancedFeatures(VK_COMPARE_OP_LESS_OR_EQUAL, VK_POLYGON_MODE_FILL,
                                     VK_CULL_MODE_NONE, true);
        
        
        
        
        PScreens.init(this, &VOverlay, "shaders/OverlayVert.spv", "shaders/OverlayFrag.spv", { &DSLOverlay });
        PScreens.setAdvancedFeatures(VK_COMPARE_OP_LESS_OR_EQUAL, VK_POLYGON_MODE_FILL,
                                     VK_CULL_MODE_NONE, true);
        
        for(int i = 0; i<OBDUN; i++){
            MEnemy[i].init(this, &VDDun, "models/veg20.mgcg", MGCG);
        }
        
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
        
        for ( int i = 0; i<OBDUN; i++){
            TEnemy[i].init(this, "textures/veg20.png");
        }
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
        PScreens.create();
        PDun.create();
        
        SC.pipelinesAndDescriptorSetsInit(DSL);
        
        
        
        for(int i=0; i<OBDUN; i++){
            DSDun[i].init(this, &DSLDun, {
                {0, UNIFORM, sizeof(DunUniformBufferObject), nullptr},
                {1, TEXTURE, 0, &TEnemy[i]},
                {2, UNIFORM, sizeof(GlobalUniformBufferObject), nullptr}
            });
        }
        
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
        PScreens.cleanup();
        PDun.cleanup();
        SC.pipelinesAndDescriptorSetsCleanup();
        
        for (int i= 0; i<OBDUN; i++){
            DSDun[i].cleanup();
        }
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
        DSLDun.cleanup();
        
        for(int i=0 ; i<OBDUN; i++){
            MEnemy[i].cleanup();
            TEnemy[i].cleanup();
            
        }
        
        P.destroy();
        POverlay.destroy();
        PScreens.destroy();
        PDun.destroy();
        SC.localCleanup();
    }
    void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {
        P.bind(commandBuffer);
        SC.populateCommandBuffer(commandBuffer, currentImage, P);
        
        
        
        
        
        PDun.bind(commandBuffer);
        for( int i = 0; i<OBDUN; i++){
            MEnemy[i].bind(commandBuffer);
            DSDun[i].bind(commandBuffer, PDun, 0, currentImage);
            vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(MEnemy[i].indices.size()), 1, 0, 0, 0);
        }
        POverlay.bind(commandBuffer);
        for (int i = 0; i < 4; i++)
        {
            MHUD[i].bind(commandBuffer);
            DSHUD[i].bind(commandBuffer, POverlay, 0, currentImage);
            vkCmdDrawIndexed(commandBuffer,
                             static_cast<uint32_t>(MHUD[i].indices.size()), 1, 0, 0, 0);
            
        }
        PScreens.bind(commandBuffer);
        for (int i = 0; i < 3; i++){
            MText[i].bind(commandBuffer);
            DSText[i].bind(commandBuffer, PScreens, 0, currentImage);
            vkCmdDrawIndexed(commandBuffer,
                             static_cast<uint32_t>(MText[i].indices.size()), 1, 0, 0, 0);
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
                            //std::cout << "Ghost Collision detected\n";
                            //std::cout <<  gx.x << ", " << gz.z << ";\n\n";
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
                //std::cout << ghostPosition.x << ", " << ghostPosition.y << ", " <<  ghostPosition.z << ";\n";

                if(Pos.y == 0){
                    for(auto &coppia : vertex_pairs){
                        if(doSegmentsIntersect(Pos, oldPos, coppia.second, coppia.first)){
                            Pos = oldPos;
                            //std::cout << "Collision detected \n\n";
                            break;
                        }
                    }
                }
                if(CheckCollision(Pos, doorPosition, 1) && score == 3){
                    doorAngle = 90.0f;
                }
                else if(CheckCollision(Pos, doorPosition, 0.5f) && score != 3){
                    Pos = oldPos;
                }
                for (auto &tree:treePosition){
                    if(CheckCollision(Pos,tree,1)){
                        Pos = oldPos;
                        break;
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
                if (CheckCollision(Pos, ghostPosition, 1)){
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
                
                

                
                
                
                static bool debounce = false;
                static int curDebounce = 0;
                
                gubo.lightType = lightDirectional;
             
                
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
                    }
                    else if ((*SC.I[i].id == "ghost")){
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
                }
                for ( int i=0 ; i< OBDUN ; i++){
                    
                    glm::vec3 scaleTrees(0.5f, 0.5f, 0.5f);
                    glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scaleTrees);
                    dunUbo[i].mMat = glm::translate(glm::mat4(1), treePosition[i]) * scaleMatrix;
                    dunUbo[i].mvpMat = ViewPrj * dunUbo[i].mMat;
                    dunUbo[i].nMat = glm::inverse(glm::transpose(dunUbo[i].mMat));
                    DSDun[i].map(currentImage, &dunUbo[i], sizeof(dunUbo[i]), 0);
                    DSDun[i].map(currentImage, &gubo, sizeof(gubo), 2);
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
