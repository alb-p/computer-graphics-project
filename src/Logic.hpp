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
	const glm::vec3 StartingPosition = glm::vec3(-41.50, 0.0, -19.5);
	
	// Camera target height and distance
	const float camHeight = 0.25;
	const float camDist = 1.5;
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