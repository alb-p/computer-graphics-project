// This has been adapted from the Vulkan tutorial
 
// TO MOVE
#define JSON_DIAGNOSTICS 1
#include "modules/Starter.hpp"
#include "modules/TextMaker.hpp"
 
 
std::vector<SingleText> outText = {
    {1, {"Third Person", "Press SPACE to change view", "", ""}, 0, 0},
    {1, {"First Person", "Press SPACE to change view", "", ""}, 0, 0},
    {1, {"Saving Screenshots. Please wait.","","",""}, 0, 0},
    {2, {"Screenshot Saved!","Press ESC to quit","",""}, 0, 0}
};
 
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
struct Vertex {
    glm::vec3 pos;
    glm::vec2 UV;
    glm::vec3 norm;
};
 
 #include "modules/Scene.hpp"
 
class A04; 
void GameLogic(A04 *A, float Ar, glm::mat4 &ViewPrj, glm::mat4 &World);

 
// MAIN !
class A04 : public BaseProject {
    protected:
    
    // Descriptor Layouts ["classes" of what will be passed to the shaders]
    DescriptorSetLayout DSL;
 
    // Vertex formats
    VertexDescriptor VD;
 
    // Pipelines [Shader couples]
    Pipeline P;
 
    Scene SC;
    glm::vec3 **deltaP;
    float *deltaA;
    float *usePitch;
 
    TextMaker txt;
    
    // Other application parameters
    int currScene = 0;
    float Ar;
    glm::vec3 Pos;
    float Yaw;
    glm::vec3 InitialPos;
    
    std::vector<std::string> subject = {"c1"};
      
    std::vector<std::string> landscape =  {"apar","prm"};
 
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
        // Descriptor Layouts [what will be passed to the shaders]
        DSL.init(this, {
                    {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS},
                    {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
                    {2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS}
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
 
        // Pipelines [Shader couples]
        P.init(this, &VD, "shaders/PhongVert.spv", "shaders/PhongFrag.spv", {&DSL});
        P.setAdvancedFeatures(VK_COMPARE_OP_LESS_OR_EQUAL, VK_POLYGON_MODE_FILL,
                                     VK_CULL_MODE_NONE, false);
 
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
 
 
        // Load Scene
        SC.init(this, &VD, DSL, P, "models/scene.json");
        
        // updates the text
        txt.init(this, &outText);
 
        // Init local variables
        Pos = glm::vec3(SC.I[SC.InstanceIds["tb"]].Wm[3]);
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
        deltaA[SC.InstanceIds["tfwl"]] =
        deltaA[SC.InstanceIds["tmwl"]] =
        deltaA[SC.InstanceIds["tbwfl"]] =
        deltaA[SC.InstanceIds["tbwrl"]] =
                        M_PI;
        usePitch[SC.InstanceIds["tfwl"]] =
        usePitch[SC.InstanceIds["tmwl"]] =
        usePitch[SC.InstanceIds["tbwfl"]] =
        usePitch[SC.InstanceIds["tbwrl"]] = -1.0f;
        usePitch[SC.InstanceIds["tfwr"]] =
        usePitch[SC.InstanceIds["tmwr"]] =
        usePitch[SC.InstanceIds["tbwfr"]] =
        usePitch[SC.InstanceIds["tbwrr"]] = 1.0f;
    }
    
    // Here you create your pipelines and Descriptor Sets!
    void pipelinesAndDescriptorSetsInit() {
        // This creates a new pipeline (with the current surface), using its shaders
        P.create();
 
        // Here you define the data set
        SC.pipelinesAndDescriptorSetsInit(DSL);
        txt.pipelinesAndDescriptorSetsInit();
    }
 
    // Here you destroy your pipelines and Descriptor Sets!
    // All the object classes defined in Starter.hpp have a method .cleanup() for this purpose
    void pipelinesAndDescriptorSetsCleanup() {
        // Cleanup pipelines
        P.cleanup();
 
        SC.pipelinesAndDescriptorSetsCleanup();
        txt.pipelinesAndDescriptorSetsCleanup();
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
        
        // Destroies the pipelines
        P.destroy();
 
        SC.localCleanup();
        txt.localCleanup();
    }
    
    // Here it is the creation of the command buffer:
    // You send to the GPU all the objects you want to draw,
    // with their buffers and textures
    
    void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {
        // binds the pipeline
        P.bind(commandBuffer);
 
/*        // binds the data set
        DS1.bind(commandBuffer, P, 0, currentImage);
                    
        // binds the model
        M1.bind(commandBuffer);
        
        // record the drawing command in the command buffer
        vkCmdDrawIndexed(commandBuffer,
                static_cast<uint32_t>(M1.indices.size()), 1, 0, 0, 0);
std::cout << M1.indices.size();
*/
        SC.populateCommandBuffer(commandBuffer, currentImage, P);
        txt.populateCommandBuffer(commandBuffer, currentImage, currScene);
    }
 
    // Here is where you update the uniforms.
    // Very likely this will be where you will be writing the logic of your application.
    void updateUniformBuffer(uint32_t currentImage) {
        
        
        if(glfwGetKey(window, GLFW_KEY_ESCAPE)) {
			glfwSetWindowShouldClose(window, GL_TRUE);
		}

		glm::mat4 ViewPrj;
		glm::mat4 WM;
		
        GameLogic(this, Ar, ViewPrj, WM);


        /*
             const float ROT_SPEED = glm::radians(120.0f);
 
 
        float deltaT;
        glm::vec3 m = glm::vec3(0.0f), r = glm::vec3(0.0f);
        bool fire = false;
        getSixAxis(deltaT, m, r, fire);
 
        static float CamPitch = glm::radians(20.0f);
        static float CamYaw   = M_PI;
        static float CamDist  = 2.0f;
        static float CamRoll  = 0.0f;
        const glm::vec3 CamTargetDelta = glm::vec3(0, 2, 0);
 
        const float MOVE_SPEED = 2.5f;
        const float ROTATION_ANGLE = glm::radians(90.0f); // 90 degrees rotation
 
        // Update object position based on input
        if (m.z > 0.0f) {  // Move forward
            Pos.x -= MOVE_SPEED * deltaT * sin(Yaw);  // Move along X based on yaw
            Pos.z -= MOVE_SPEED * deltaT * cos(Yaw);  // Move along Z based on yaw
        }
        else if (m.z < 0.0f) {  // Move backward
            Pos.x += MOVE_SPEED * deltaT * sin(Yaw);
            Pos.z += MOVE_SPEED * deltaT * cos(Yaw);
        }
        else if (m.x < 0.0f) {  // Turn left
            Yaw -= ROTATION_ANGLE;
            if (Yaw < 0.0f) Yaw += 2 * M_PI;  // Normalize yaw to stay within 0 - 2π
        }
        else if (m.x > 0.0f) {  // Turn right
            Yaw += ROTATION_ANGLE;
            if (Yaw > 2 * M_PI) Yaw -= 2 * M_PI;  // Normalize yaw to stay within 0 - 2π
        }
 
        // Camera logic (if using a camera, you might want to adjust this)
        CamYaw += ROT_SPEED * deltaT * r.y;
        CamPitch -= ROT_SPEED * deltaT * r.x;
        CamRoll -= ROT_SPEED * deltaT * r.z;
        CamDist -= MOVE_SPEED * deltaT * m.y;
        
        //CamYaw = (CamYaw < 0.0f ? 0.0f : (CamYaw > 2 * M_PI ? 2 * M_PI : CamYaw));
        
        CamYaw = fmod(CamYaw, 2 * M_PI);
        if (CamYaw < 0.0f) {
            CamYaw += 2 * M_PI;
        }
        
        CamPitch = (CamPitch < 0.0f ? 0.0f : (CamPitch > M_PI_2 - 0.01f ? M_PI_2 - 0.01f : CamPitch));
        CamRoll = (CamRoll < -M_PI ? -M_PI : (CamRoll > M_PI ? M_PI : CamRoll));
 
        // Compute Camera Position
        glm::vec3 CamTarget = Pos + glm::vec3(glm::rotate(glm::mat4(1), Yaw, glm::vec3(0, 1, 0)) *
                                 glm::vec4(CamTargetDelta, 1));
        glm::vec3 CamPos = CamTarget + glm::vec3(glm::rotate(glm::mat4(1), Yaw + CamYaw, glm::vec3(0, 1, 0)) *
                                 glm::rotate(glm::mat4(1), -CamPitch, glm::vec3(1, 0, 0)) *
                                 glm::vec4(0, 0, CamDist, 1));
        
        const float lambdaCam = 10.0f;
        static glm::vec3 dampedCamPos = CamPos;
        dampedCamPos = CamPos * (1 - exp(-lambdaCam * deltaT)) +
                       dampedCamPos * exp(-lambdaCam * deltaT);
 
        // Create View-Projection matrix
        glm::mat4 M = MakeViewProjectionLookAt(dampedCamPos, CamTarget, glm::vec3(0, 1, 0), CamRoll, glm::radians(90.0f), Ar, 0.1f, 500.0f);
 
        glm::mat4 ViewPrj = M;

        */



        UniformBufferObject ubo{};
        glm::mat4 baseTr = glm::mat4(1.0f);
 
        // Update global uniforms
        GlobalUniformBufferObject gubo{};
        gubo.lightDir = glm::vec3(cos(glm::radians(135.0f)), sin(glm::radians(135.0f)), 0.0f);
        gubo.lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        gubo.eyePos = glm::vec3(100.0, 100.0, 100.0);

        // gubo.eyeDir = glm::vec4(0);
        // gubo.eyeDir.w = 1.0;
 
        // Draw the object in the scene
        for (std::vector<std::string>::iterator it = subject.begin(); it != subject.end(); it++) {
            int i = SC.InstanceIds[it->c_str()];
            glm::vec3 dP = glm::vec3(glm::rotate(glm::mat4(1), Yaw, glm::vec3(0, 1, 0)) *
                                     glm::vec4(*deltaP[i], 1));
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
            ubo.mMat = SC.I[i].Wm * baseTr;
            ubo.mvpMat = ViewPrj * ubo.mMat;
            ubo.nMat = glm::inverse(glm::transpose(ubo.mMat));
 
            SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
            SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);
        }
    }
 
};


void GameLogic(A04 *A, float Ar, glm::mat4 &ViewPrj, glm::mat4 &World) {
	// The procedure must implement the game logic  to move the character in third person.
	// Input:
	// <Assignment07 *A> Pointer to the current assignment code. Required to read the input from the user
	// <float Ar> Aspect ratio of the current window (for the Projection Matrix)
	// Output:
	// <glm::mat4 &ViewPrj> the view-projection matrix of the camera
	// <glm::mat4 &World> the world matrix of the object
	// Parameters
	// Camera FOV-y, Near Plane and Far Plane
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
	const float MOVE_SPEED = 2.0f;

	// Integration with the timers and the controllers
	// returns:
	// <float deltaT> the time passed since the last frame
	// <glm::vec3 m> the state of the motion axes of the controllers (-1 <= m.x, m.y, m.z <= 1)
	// <glm::vec3 r> the state of the rotation axes of the controllers (-1 <= r.x, r.y, r.z <= 1)
	// <bool fire> if the user has pressed a fire button (not required in this assginment)
	float deltaT;
	glm::vec3 m = glm::vec3(0.0f), r = glm::vec3(0.0f);
	bool fire = false;
	A->getSixAxis(deltaT, m, r, fire);

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
	glm::vec3 ux = glm::rotate(glm::mat4(1.0f), Yaw, glm::vec3(0,1,0)) * glm::vec4(1,0,0,1);
	glm::vec3 uy = glm::vec3(0,1,0);
	glm::vec3 uz = glm::rotate(glm::mat4(1.0f), Yaw, glm::vec3(0,1,0)) * glm::vec4(0,0,1,1);
	Pos = Pos + MOVE_SPEED * m.x * ux * deltaT;
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

	std::cout << Pos.x << ", " << Pos.y << ", " << Pos.z << ", " << Yaw << ", " << Pitch << ", " << Roll << "\n";

	// Final world matrix computaiton
	World = glm::translate(glm::mat4(1.0), Pos) * glm::rotate(glm::mat4(1.0f), Yaw, glm::vec3(0,1,0));

	// Projection
	glm::mat4 Prj = glm::perspective(FOVy, Ar, nearPlane, farPlane);
	Prj[1][1] *= -1;

	// View
	// Target
	glm::vec3 target = Pos + glm::vec3(0.0f, camHeight, 0.0f);
	target = (World * glm::vec4(0,0,0,1)) + (0,0,camHeight);
	

	// Camera position, depending on Yaw parameter, but not character direction
	glm::vec3 cameraPos = World * glm::vec4(0.0f, camHeight + (camDist * sin(Pitch)), (camDist * cos(Pitch)), 1.0);
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
	
	}

 
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