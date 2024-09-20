// This has been adapted from the Vulkan tutorial
 
// TO MOVE
#define JSON_DIAGNOSTICS 1
#include "modules/Starter.hpp"
#include "modules/TextMaker.hpp"
#include <string>
#include <iostream>
#include <format>

enum GameState {
    notStarted,
    playing,
    ended
};

template<typename ... Args>
std::string string_format(const std::string& format, Args ... args)

{
    int size_s = std::snprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
    if (size_s <= 0) { throw std::runtime_error("Error during formatting."); }
    auto size = static_cast<size_t>(size_s);
    std::unique_ptr<char[]> buf(new char[size]);
    std::snprintf(buf.get(), size, format.c_str(), args ...);
    return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}

struct OverlayUniformBlock {
    alignas(4) float visible;
};
struct VertexOverlay {
    glm::vec2 pos;
    glm::vec2 UV;
};

std::vector<SingleText> outText = {
    {1, {"Current Score: ", "", "", ""}, 0, 0}
};

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
void changeText(SingleText &textObject, int index, const char* newString) {
    
   //std::strcpy(textObject.l[index], (char*) newString);
    
   textObject.l[index] = newString;
}
bool stopRendering = true;
// The uniform buffer object used in this example
struct UniformBufferObject {
    alignas(16) glm::mat4 mvpMat;
    alignas(16) glm::mat4 mMat;
    alignas(16) glm::mat4 nMat;
};
 
struct GlobalUniformBufferObject {
    alignas(16) glm::vec3 lightDir;
    alignas(16) glm::vec4 lightColor;
    alignas(16) glm::vec3 eyePos;
    //alignas(16) glm::vec4 eyeDir;
};
 
 
 
// The vertices data structures
// Example
/*struct Vertex {
    glm::vec3 pos;
    glm::vec2 UV;
    glm::vec3 norm;
};*/
struct Vertex {
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec2 UV;
    glm::vec4 tan;
    Vertex(glm::vec3 &pos, glm::vec3 &norm, glm::vec2 &UV, glm::vec4 &tan): pos(pos), norm(norm), UV(UV), tan(tan){};
};
std::vector<unsigned char> serializeVertices(const std::vector<VertexOverlay>& vertices) {
    std::vector<unsigned char> buffer;
    buffer.resize(vertices.size() * sizeof(VertexOverlay));
    memcpy(buffer.data(), vertices.data(), buffer.size());
    return buffer;
}
#include "modules/Scene.hpp"
 
// #include "WVP.hpp"

class CGproj; 
void GameLogic(CGproj *A, float Ar, glm::mat4 &ViewPrj, glm::mat4 &World);

 
// MAIN !
class CGproj : public BaseProject {
protected:
    GameState game_state = notStarted;
    
    // Descriptor Layouts ["classes" of what will be passed to the shaders]
    DescriptorSetLayout DSL, DSLOverlay;
    
    // Vertex formats
    VertexDescriptor VD, VOverlay;
    
    // Pipelines [Shader couples]
    Pipeline P, POverlay;
    
    Model MText[3], MHUD[4];
    DescriptorSet DSText[3], DSHUD[4];
    Texture TText[3], THUD[4];
    OverlayUniformBlock uboText[3], uboHUD[4];
    Scene SC;
    
    glm::vec3 **deltaP;
    float *deltaA;
    float *usePitch;
    
    //TextMaker txt;
    
    // Other application parameters
    int currScene = 0;
    float Ar;
    glm::vec3 Pos;
    float Yaw;
    glm::vec3 InitialPos;
    int score = 0;
    std::vector<std::string> landscape =  {"pavimento","apar", "maze", "grave"};
    
    std::vector<std::string> subject = {"c1"};
    glm::vec3 item1Position =  glm::vec3(-15.0, 0.0, -15.0);
    glm::vec3 item2Position =  glm::vec3(-8.0, 0.0, -20.0);
    glm::vec3 item3Position =  glm::vec3(0.0, 0.0, -35.0);
    
    glm::vec3 trap1Position =  glm::vec3(-10.0, 0.0, -10.0);

    std::vector<glm::vec3> itemsPositions = {item1Position, item2Position, item3Position};
    
    CollectibleItem object1 = CollectibleItem(item1Position,false,"objectToCollect");
    CollectibleItem object2 = CollectibleItem(item2Position,false,"objectToCollect2");
    CollectibleItem object3 = CollectibleItem(item3Position,false,"objectToCollect3");
    
    
    bool gameStarted = false;
    bool gameWon = false;

    // Here you set the main application parameters
    void setWindowParameters() {
        // window size, titile and initial background
        windowWidth = 1280;
        windowHeight = 720;
        windowTitle = "PAC-MAZE";
        windowResizable = GLFW_TRUE;
        initialBackgroundColor = {0.0f, 0.85f, 1.0f, 1.0f};
        
        // Descriptor pool sizes
        uniformBlocksInPool = 25 * 2 + 2;
        texturesInPool = 29 + 1;
        setsInPool = 29 + 1;
        
        Ar = 4.0f / 3.0f;
    }
    
    // What to do when the window changes size
    void onWindowResize(int w, int h) {
        std::cout << "Window resized to: " << w << " x " << h << "\n";
        Ar = (float)w / (float)h;
    }
    
    // Here you load and setup all your Vulkan Models and Texutures.
    // Here you also create your Descriptor set layouts and load the shaders for the pipelines
    void localInit() {
        
        glm::vec2 anchor = glm::vec2(-0.7f);
        float h = 0.45;
        float w = 0.55;
        // Descriptor Layouts [what will be passed to the shaders]
        DSL.init(this, {
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS},
            {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
            {2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS}
        });
        DSLOverlay.init(this, {
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
                {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
            });
        // Vertex descriptors
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
        
        // Pipelines [Shader couples]
        P.init(this, &VD, "shaders/PhongVert.spv", "shaders/PhongFrag.spv", {&DSL});
        P.setAdvancedFeatures(VK_COMPARE_OP_LESS_OR_EQUAL, VK_POLYGON_MODE_FILL,
                              VK_CULL_MODE_NONE, false);
        POverlay.init(this, &VOverlay, "shaders/OverlayVert.spv", "shaders/OverlayFrag.spv", { &DSLOverlay });
        POverlay.setAdvancedFeatures(VK_COMPARE_OP_LESS_OR_EQUAL, VK_POLYGON_MODE_FILL,
            VK_CULL_MODE_NONE, true);
        
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
        // Init local variables
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
    
    // Here you create your pipelines and Descriptor Sets!
    void pipelinesAndDescriptorSetsInit() {
        // This creates a new pipeline (with the current surface), using its shaders
        P.create();
        POverlay.create();

        SC.pipelinesAndDescriptorSetsInit(DSL);
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
    
    // Here you destroy your pipelines and Descriptor Sets!
    // All the object classes defined in Starter.hpp have a method .cleanup() for this purpose
    void pipelinesAndDescriptorSetsCleanup() {
        // Cleanup pipelines
        P.cleanup();
        POverlay.cleanup();

        SC.pipelinesAndDescriptorSetsCleanup();
        for (int i = 0; i < 3; i++){
            DSText[i].cleanup();
        }
        for (int i = 0; i < 4; i++){
            DSHUD[i].cleanup();
        }
    }
    
    // Here you destroy all the Models, Texture and Desc. Set Layouts you created!
    // All the object classes defined in Starter.hpp have a method .cleanup() for this purpose
    // You also have to destroy the pipelines: since they need to be rebuilt, they have two different
    // methods: .cleanup() recreates them, while .destroy() delete them completely
    void localCleanup() {
        for(int i=0; i < SC.InstanceCount; i++) {
            delete deltaP[i];
        }
        free(deltaP);
        free(deltaA);
        free(usePitch);
        
        // Cleanup descriptor set layouts
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

        // Destroies the pipelines
        P.destroy();
        POverlay.destroy();

        SC.localCleanup();
    }
    
    // Here it is the creation of the command buffer:
    // You send to the GPU all the objects you want to draw,
    // with their buffers and textures
    
    void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {
        P.bind(commandBuffer);
        
        SC.populateCommandBuffer(commandBuffer, currentImage, P);
        
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
    
    void updateUniformBuffer(uint32_t currentImage) {
        
        
        if(glfwGetKey(window, GLFW_KEY_ESCAPE)) {
            glfwSetWindowShouldClose(window, GL_TRUE);
        }
        
        glm::mat4 ViewPrj;
        glm::mat4 WM;
        const float FOVy = glm::radians(45.0f);
        const float nearPlane = 0.1f;
        const float farPlane = 100.f;
        // Player starting point
        const glm::vec3 StartingPosition = glm::vec3(-1.50, 0.0, -0.5);
        // Camera target height and distance
        const float camHeight = 1.0f;
        const float camDist = 3.5f;
        // Camera Pitch limits
        const float minPitch = glm::radians(-8.75f);
        const float maxPitch = glm::radians(60.0f);
        // Rotation and motion speed
        const float ROT_SPEED = glm::radians(120.0f);
        const float MOVE_SPEED = 5.0f;
        float deltaT;
        glm::vec3 m = glm::vec3(0.0f), r = glm::vec3(0.0f);
        bool fire = false;
        bool start = false;
        
        bool hideMaze = false;
        bool mazeVisible = true;
        getSixAxis(deltaT, m, r, fire, start, hideMaze);
        
        // Game Logic implementation
        // Current Player Position - static variables make sure that its value remain unchanged in subsequent calls to the procedure
        static glm::vec3 Pos = StartingPosition;
        
        
        // To be done in the assignment
        
        glm::mat4 ViewPrjOld = glm::mat4(1);
        
        //The view matrix should be computed using the LookAt technique
        static float Yaw = glm::radians(0.0f);
        static float Pitch = glm::radians(0.0f);
        static float Roll = glm::radians(0.0f);
        
        UniformBufferObject ubo{};
        GlobalUniformBufferObject gubo{};

        if(start){
            gameStarted = true;
            game_state = playing;
            uboText[0].visible = 0.0f;
            DSText[0].map(currentImage, &uboText[0], sizeof(uboText[0]), 0);
        }
        
        
        glm::mat4 baseTr = glm::mat4(1.0f);
        // Projection
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
                
                if(score == 3){
                    gameWon = true;
                    game_state = ended;
                    break;
                }
                
                ux = glm::rotate(glm::mat4(1.0f), Yaw, glm::vec3(0,1,0)) * glm::vec4(1,0,0,1);
                uy = glm::vec3(0,1,0);
                uz = glm::rotate(glm::mat4(1.0f), Yaw, glm::vec3(0,1,0)) * glm::vec4(0,0,1,1);
                Pos = Pos + MOVE_SPEED * m.x * ux * deltaT;
                Pos = Pos + MOVE_SPEED * m.y * uy * deltaT;
                Pos.y = Pos.y < 0.0f ? 0.0f : Pos.y;
                Pos = Pos + MOVE_SPEED * m.z * uz * deltaT;
                Yaw = Yaw - ROT_SPEED * deltaT * r.y;
                Pitch = Pitch + ROT_SPEED * deltaT * r.x;
                Pitch  =  Pitch < minPitch ? minPitch :
                (Pitch > maxPitch ? maxPitch : Pitch);
                Roll   = Roll  - ROT_SPEED * deltaT * r.z;
                Roll   = Roll < glm::radians(-175.0f) ? glm::radians(-175.0f) :
                (Roll > glm::radians( 175.0f) ? glm::radians( 175.0f) : Roll);
        
                if (!object1.isCollected && CheckCollision(Pos, item1Position, 3)) {
                    //std::cout << Pos.x << ", " << Pos.z << ", item 1 collected \n";
                    object1.isCollected = true;
                    //PlaySoundEffect("collect.wav");
                }
                if (!object2.isCollected && CheckCollision(Pos, item2Position, 3)) {
                    //std::cout << Pos.x << ", " << Pos.z << ", item 2 collected \n";
                    object2.isCollected = true;
                    //PlaySoundEffect("collect.wav");
                }
                if (!object3.isCollected && CheckCollision(Pos, item3Position, 3)) {
                    //std::cout << Pos.x << ", " << Pos.z << ", item 3 collected \n";
                    object3.isCollected = true;
                    //PlaySoundEffect("collect.wav");
                }
                if(CheckCollision(Pos, trap1Position, 3)){
                    gameWon = false;
                    game_state = ended;
                }
                WM = glm::translate(glm::mat4(1.0), Pos) * glm::rotate(glm::mat4(1.0f), Yaw, glm::vec3(0,1,0));
                
                // Projection
                Prj = glm::perspective(FOVy, Ar, nearPlane, farPlane);
                Prj[1][1] *= -1;
                
                // View
                // Target
                target = Pos + glm::vec3(0.0f, camHeight, 0.0f);
                //target = (WM * glm::vec4(0,0,0,1)) + (static_cast<void>(0),static_cast<void>(0),camHeight);
                
                
                // Camera position, depending on Yaw parameter, but not character direction
                cameraPos = WM * glm::vec4(0.0f, camHeight + (camDist * sin(Pitch)), (camDist * cos(Pitch)), 1.0);
                // Final view matrix
                //lookAt:
                //eye – Position of the camera
                //center – Position where the camera is looking at
                //up – Normalized up vector, how the camera is oriented. Typically (0, 0, 1)
                View = glm::rotate(glm::mat4(1.0f), -Roll, glm::vec3(0,0,1)) * glm::lookAt(cameraPos, target, glm::vec3(0,1,0));
                
                ViewPrj = Prj * View;
                
                if (ViewPrjOld == glm::mat4(1))
                    ViewPrjOld = ViewPrj;
                ViewPrj = ViewPrjOld * exp(-lambda * deltaT) + ViewPrj * (1 - exp(-lambda * deltaT));
                ViewPrjOld = ViewPrj;
                
                
                // Update global uniforms
                gubo.lightDir = glm::vec3(cos(glm::radians(135.0f)), sin(glm::radians(135.0f)), 0.0f);
                gubo.lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
                gubo.eyePos = glm::vec3(100.0, 100.0, 100.0);
                
                // gubo.eyeDir = glm::vec4(0);
                // gubo.eyeDir.w = 1.0;
                
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
                    
                    if (*SC.I[i].id == "objectToCollect") {
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
                            
                            ubo.mMat = SC.I[i].Wm * translationMatrix * continuousRotation * rotationMatrix * baseTr;                    ubo.mvpMat = ViewPrj * ubo.mMat;
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
                    else{ //praticamente dentro questo else fa solo sky e pavimento
                        ubo.mMat = SC.I[i].Wm * baseTr;
                        ubo.mvpMat = ViewPrj * ubo.mMat;
                        ubo.nMat = glm::inverse(glm::transpose(ubo.mMat));
                        
                        SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
                        SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);
                    }
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


// This is the main: probably you do not need to touch this!
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
