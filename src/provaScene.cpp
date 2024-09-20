// This has been adapted from the Vulkan tutorial
 
// TO MOVE
#define JSON_DIAGNOSTICS 1
#include "modules/Starter.hpp"
#include "modules/TextMaker.hpp"
#include <string>
#include <iostream>
#include <format>

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

class A04; 
void GameLogic(A04 *A, float Ar, glm::mat4 &ViewPrj, glm::mat4 &World);

 
// MAIN !
class A04 : public BaseProject {
protected:
    
    // Descriptor Layouts ["classes" of what will be passed to the shaders]
    DescriptorSetLayout DSL, DSLOverlay;
    
    // Vertex formats
    VertexDescriptor VD, VOverlay;
    
    // Pipelines [Shader couples]
    Pipeline P, POverlay;
    
    Model MText, MHUD[4];
    DescriptorSet DSText, DSHUD[4];
    Texture TText, THUD[4];
    OverlayUniformBlock uboText, uboHUD[4];

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
    std::vector<std::string> landscape =  {"pavimento","apar", "apar3"};
    
    std::vector<std::string> subject = {"c1"};
    glm::vec3 item1Position =  glm::vec3(-15.0, 0.0, -15.0);
    glm::vec3 item2Position =  glm::vec3(-8.0, 0.0, -20.0);
    glm::vec3 item3Position =  glm::vec3(0.0, 0.0, -35.0);
    std::vector<glm::vec3> itemsPositions = {item1Position, item2Position, item3Position};
    
    CollectibleItem object1 = CollectibleItem(item1Position,false,"objectToCollect");
    CollectibleItem object2 = CollectibleItem(item2Position,false,"objectToCollect2");
    CollectibleItem object3 = CollectibleItem(item3Position,false,"objectToCollect3");
    
    
    bool gameStarted = false;

    // Here you set the main application parameters
    void setWindowParameters() {
        // window size, titile and initial background
        windowWidth = 800;
        windowHeight = 600;
        windowTitle = "A04 - World View Projection";
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
        // Models, textures and Descriptors (values assigned to the uniforms)
        /*        std::vector<Vertex> vertices = {
         {{-100.0,0.0f,-100.0}, {0.0f,0.0f}, {0.0f,1.0f,0.0f}},
         {{-100.0,0.0f, 100.0}, {0.0f,1.0f}, {0.0f,1.0f,0.0f}},
         {{ 100.0,0.0f,-100.0}, {1.0f,0.0f}, {0.0f,1.0f,0.0f}},
         {{ 100.0,0.0f, 100.0}, {1.0f,1.0f}, {0.0f,1.0f,0.0f}}};
         M1.vertices = std::vector<unsigned char>(vertices.size()*sizeof(Vertex), 0);
         memcpy(&vertices[0], &M1.vertices[0], vertices.size()*sizeof(Vertex));
         M1.indices = {0, 1, 2,    1, 3, 2};
         M1.initMesh(this, &VD); */
        
        std::vector<VertexOverlay> vertexData = {
            {{anchor.x, anchor.y}, {0.0f, 0.0f}},
            {{anchor.x, anchor.y + h}, {0.0f, 1.0f}},
            {{anchor.x + w, anchor.y}, {1.0f, 0.0f}},
            {{anchor.x + w, anchor.y + h}, {1.0f, 1.0f}}
        };

        MText.vertices = serializeVertices(vertexData);

       
        MText.indices = { 0, 1, 2,    1, 2, 3 };
        MText.initMesh(this, &VOverlay);
        anchor = glm::vec2(-0.95, -0.95);
        h = 0.25;
        w = 0.1;
        for (int i = 0; i < 4; i++)
        {
            
            vertexData = { {{anchor.x, anchor.y}, {0.0f,0.0f}}, {{anchor.x, anchor.y + h}, {0.0f,1.0f}},
                             {{anchor.x + w, anchor.y}, {1.0f,0.0f}}, {{ anchor.x + w, anchor.y + h}, {1.0f,1.0f}} };
            MHUD[i].vertices = serializeVertices(vertexData);
            MHUD[i].indices = { 0, 1, 2,    1, 2, 3 };
            MHUD[i].initMesh(this, &VOverlay);
        }
        
        // Load Scene
        SC.init(this, &VD, DSL, P, "models/scene.json");
        
        
        TText.init(this, "textures/provaT.png");
        for (int i = 0; i < 4; i++)
        {
            THUD[i].init(this, string_format("textures/life%d.png", i ).c_str());
        }
        // updates the text
        //txt.init(this, &outText);
        
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

        // Here you define the data set
        SC.pipelinesAndDescriptorSetsInit(DSL);
        //txt.pipelinesAndDescriptorSetsInit();
        
        DSText.init(this, &DSLOverlay, {
                {0, UNIFORM, sizeof(OverlayUniformBlock), nullptr},
                {1, TEXTURE, 0, &TText}
            });
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
        DSText.cleanup();
        for (int i = 0; i < 4; i++)
        {
            DSHUD[i].cleanup();
        }
        //txt.pipelinesAndDescriptorSetsCleanup();
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
        TText.cleanup();
        for (int i = 0; i < 4; i++)
        {
            THUD[i].cleanup();
        }
        MText.cleanup();
        for (int i = 0; i < 4; i++)
        {
            MHUD[i].cleanup();
        }
        DSLOverlay.cleanup();

        // Destroies the pipelines
        P.destroy();
        POverlay.destroy();

        SC.localCleanup();
        //txt.localCleanup();
    }
    
    // Here it is the creation of the command buffer:
    // You send to the GPU all the objects you want to draw,
    // with their buffers and textures
    
    void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {
        P.bind(commandBuffer);
        
        SC.populateCommandBuffer(commandBuffer, currentImage, P);
        //txt.populateCommandBuffer(commandBuffer, currentImage, currScene);
        
        POverlay.bind(commandBuffer);
        MText.bind(commandBuffer);
        DSText.bind(commandBuffer, POverlay, 0, currentImage);
        vkCmdDrawIndexed(commandBuffer,
            static_cast<uint32_t>(MText.indices.size()), 1, 0, 0, 0);
        
        for (int i = 0; i < 4; i++)
        {
            MHUD[i].bind(commandBuffer);
            DSHUD[i].bind(commandBuffer, POverlay, 0, currentImage);
            vkCmdDrawIndexed(commandBuffer,
                static_cast<uint32_t>(MHUD[i].indices.size()), 1, 0, 0, 0);

        }
    }
    
    // Here is where you update the uniforms.
    // Very likely this will be where you will be writing the logic of your application.
    glm::vec3 posToCheck = glm::vec3(Pos.x, Pos.y, Pos.z);
    std::vector<CollectibleItem> collectibleItems = {object1, object2, object3};
    
    void updateUniformBuffer(uint32_t currentImage) {
        /*
        score = 0;
        
        for(auto &o : collectibleItems){
            if(o.isCollected){
                score++;
            }
        }*/
        if(glfwGetKey(window, GLFW_KEY_ESCAPE)) {
            glfwSetWindowShouldClose(window, GL_TRUE);
        }
        
        
        
        if(object1.isCollected == false && !(count(landscape.begin(), landscape.end(), object1.name)>0)){
            landscape.push_back(object1.name);
        }
        if(object2.isCollected == false && !(count(landscape.begin(), landscape.end(), object2.name)>0)){
            landscape.push_back(object2.name);
        }
        if(object3.isCollected == false && !(count(landscape.begin(), landscape.end(), object3.name)>0)){
            landscape.push_back(object3.name);
        }
        
        glm::mat4 ViewPrj;
        glm::mat4 WM;
        
        const float FOVy = glm::radians(45.0f);
        const float nearPlane = 0.1f;
        const float farPlane = 100.f;
        
        // Player starting point
        const glm::vec3 StartingPosition = glm::vec3(-1.50, 0.0, -0.5);
        
        // Camera target height and distance
        const float camHeight = 1.25;
        const float camDist = 5;
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
        getSixAxis(deltaT, m, r, fire, start);
        
        // Game Logic implementation
        // Current Player Position - static variables make sure that its value remain unchanged in subsequent calls to the procedure
        static glm::vec3 Pos = StartingPosition;
        
        
        // To be done in the assignment
        
        glm::mat4 ViewPrjOld = glm::mat4(1);
        
        //The view matrix should be computed using the LookAt technique
        static float Yaw = glm::radians(0.0f);
        static float Pitch = glm::radians(0.0f);
        static float Roll = glm::radians(0.0f);
        /*
         glm::mat4 ViewPrj = glm::mat4(1);
         glm::mat4 World = glm::mat4(1);
         */
        // World
        // Position
        uboText.visible = 1.0f;;
        DSText.map(currentImage, &uboText, sizeof(uboText), 0);
        if(start){
            gameStarted = true;
            uboText.visible = 0.0f;;
            DSText.map(currentImage, &uboText, sizeof(uboText), 0);
        }
        if(gameStarted){
            uboText.visible = 0.0f;;
            DSText.map(currentImage, &uboText, sizeof(uboText), 0);
            
            
            glm::vec3 ux = glm::rotate(glm::mat4(1.0f), Yaw, glm::vec3(0,1,0)) * glm::vec4(1,0,0,1);
            glm::vec3 uy = glm::vec3(0,1,0);
            glm::vec3 uz = glm::rotate(glm::mat4(1.0f), Yaw, glm::vec3(0,1,0)) * glm::vec4(0,0,1,1);
            Pos = Pos + MOVE_SPEED * m.x * ux * deltaT;
            
            //PER MODIFICARE VELOCITA DEL SALTO MODIFICARE QUEL 20
            
            //Pos +=   uy *  (float(sin(glfwGetTime()*20))/3) * deltaT;
            Pos = Pos + MOVE_SPEED * m.y * uy * deltaT;
            //, if the y-coordinate of the player's position is less than zero, it's set to zero.
            Pos.y = Pos.y < 0.0f ? 0.0f : Pos.y;
            Pos = Pos + MOVE_SPEED * m.z * uz * deltaT;
            // Rotation
            Yaw = Yaw - ROT_SPEED * deltaT * r.y;
            Pitch = Pitch + ROT_SPEED * deltaT * r.x;
            Pitch  =  Pitch < minPitch ? minPitch :
            (Pitch > maxPitch ? maxPitch : Pitch);
            Roll   = Roll  - ROT_SPEED * deltaT * r.z;
            Roll   = Roll < glm::radians(-175.0f) ? glm::radians(-175.0f) :
            (Roll > glm::radians( 175.0f) ? glm::radians( 175.0f) : Roll);
        }
        //        std::cout << Pos.x << ", " << Pos.y << ", " << Pos.z << "\n";
        
        if (!object1.isCollected && CheckCollision(Pos, item1Position, 3)) {
            std::cout << Pos.x << ", " << Pos.z << ", item 1 collected \n";
            object1.isCollected = true;
            //PlaySoundEffect("collect.wav");
        }
        if (!object2.isCollected && CheckCollision(Pos, item2Position, 3)) {
            std::cout << Pos.x << ", " << Pos.z << ", item 2 collected \n";
            object2.isCollected = true;
            //PlaySoundEffect("collect.wav");
        }
        if (!object3.isCollected && CheckCollision(Pos, item3Position, 3)) {
            std::cout << Pos.x << ", " << Pos.z << ", item 3 collected \n";
            object3.isCollected = true;
            //PlaySoundEffect("collect.wav");
        }
        
        if(glfwGetKey(window, GLFW_KEY_ESCAPE)) {
            glfwSetWindowShouldClose(window, GL_TRUE);
        }
        // Final world matrix computaiton
        WM = glm::translate(glm::mat4(1.0), Pos) * glm::rotate(glm::mat4(1.0f), Yaw, glm::vec3(0,1,0));
        
        // Projection
        glm::mat4 Prj = glm::perspective(FOVy, Ar, nearPlane, farPlane);
        Prj[1][1] *= -1;
        
        // View
        // Target
        glm::vec3 target = Pos + glm::vec3(0.0f, camHeight, 0.0f);
        target = (WM * glm::vec4(0,0,0,1)) + (static_cast<void>(0),static_cast<void>(0),camHeight);
        
        
        // Camera position, depending on Yaw parameter, but not character direction
        glm::vec3 cameraPos = WM * glm::vec4(0.0f, camHeight + (camDist * sin(Pitch)), (camDist * cos(Pitch)), 1.0);
        // Final view matrix
        //lookAt:
        //eye – Position of the camera
        //center – Position where the camera is looking at
        //up – Normalized up vector, how the camera is oriented. Typically (0, 0, 1)
        glm::mat4 View = glm::rotate(glm::mat4(1.0f), -Roll, glm::vec3(0,0,1)) *
        glm::lookAt(cameraPos, target, glm::vec3(0,1,0));
        
        ViewPrj = Prj * View;
        
        float lambda = 10;
        if (ViewPrjOld == glm::mat4(1))
            ViewPrjOld = ViewPrj;
        ViewPrj = ViewPrjOld * exp(-lambda * deltaT) + ViewPrj * (1 - exp(-lambda * deltaT));
        ViewPrjOld = ViewPrj;
        
        UniformBufferObject ubo{};
        glm::mat4 baseTr = glm::mat4(1.0f);
        
        // Update global uniforms
        GlobalUniformBufferObject gubo{};
        gubo.lightDir = glm::vec3(cos(glm::radians(135.0f)), sin(glm::radians(135.0f)), 0.0f);
        gubo.lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        gubo.eyePos = glm::vec3(100.0, 100.0, 100.0);
        
        // gubo.eyeDir = glm::vec4(0);
        // gubo.eyeDir.w = 1.0;
        
        // Draw the subject in the scene
        for (std::vector<std::string>::iterator it = subject.begin(); it != subject.end(); it++) {
            int i = SC.InstanceIds[it->c_str()];
            
            //ubo.mMat = MakeWorld(Pos + dP, Yaw + deltaA[i], usePitch[i] , 0) * baseTr;
            
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
                    
                    glm::vec3 floatingY = glm::vec3(1.0f,(glm::abs(2+float(sin(glfwGetTime()*10))) * deltaT),1.0f);
                    glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), floatingY);
                    ubo.mMat = SC.I[i].Wm * translationMatrix *baseTr;
                    ubo.mvpMat = ViewPrj * ubo.mMat;
                    ubo.nMat = glm::inverse(glm::transpose(ubo.mMat));
                    
                    SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
                    SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);
                }
            }else if(*SC.I[i].id == "objectToCollect2") {
                if (object2.isCollected) {
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
                    glm::vec3 floatingY = glm::vec3(1.0f,(glm::abs(2+float(sin(glfwGetTime()*10))) * deltaT),1.0f);
                    glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), floatingY);
                    ubo.mMat = SC.I[i].Wm * translationMatrix *baseTr;
                    ubo.mvpMat = ViewPrj * ubo.mMat;
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
                } else {
                    // Normal transformation for uncollected objects
                    glm::vec3 floatingY = glm::vec3(1.0f,(glm::abs(10+float(sin(glfwGetTime()*10))) * deltaT),1.0f);
                    glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), floatingY);
                    ubo.mMat = SC.I[i].Wm * translationMatrix *baseTr;
                    ubo.mvpMat = ViewPrj * ubo.mMat;
                    ubo.nMat = glm::inverse(glm::transpose(ubo.mMat));
                    
                    SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
                    SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);
                }
            }
            else{
                ubo.mMat = SC.I[i].Wm * baseTr;
                ubo.mvpMat = ViewPrj * ubo.mMat;
                ubo.nMat = glm::inverse(glm::transpose(ubo.mMat));
                
                SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
                SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);
            }
        }
        std::string displayText = "Score: " + std::to_string(score);
        //displayText.c_str();
        
        //changeText(outText[0], 0, displayText.c_str());
       

        for (int i = 0; i < 4; i++)
        {
            uboHUD[i].visible = 0.0f;
            DSHUD[i].map(currentImage, &uboHUD[i], sizeof(uboHUD[i]), 0);
        }
        uboHUD[2].visible = 1.0f;
        DSHUD[2].map(currentImage, &uboHUD[2], sizeof(uboHUD[2]), 0);
       // txt.updateText(&outText);

    }

};


// This is the main: probably you do not need to touch this!
int main() {
    A04 app;
 
    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
 
    return EXIT_SUCCESS;
}
