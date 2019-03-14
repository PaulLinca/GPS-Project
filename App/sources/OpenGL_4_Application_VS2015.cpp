#define GLEW_STATIC
#define glCheckError() glCheckError_(__FILE__, __LINE__)
#define TINYOBJLOADER_IMPLEMENTATION

#include <iostream>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/matrix_inverse.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "GLEW/glew.h"
#include "GLFW/glfw3.h"
#include <string>
#include "Shader.hpp"
#include "Camera.hpp"
#include "SkyBox.hpp"
#include "Model3D.hpp"
#include "Mesh.hpp"

//camera animation
bool cameraAnimation = false;
float cameraAngle;
double currentTime;

//window
int glWindowWidth = 1600;
int glWindowHeight = 900;
int retina_width, retina_height;
GLFWwindow* glWindow = NULL;

//shadow
const GLuint SHADOW_WIDTH = 4*2048, SHADOW_HEIGHT = 4*2048;

//matrices
glm::mat4 model;
GLuint modelLoc;
glm::mat4 view;
GLuint viewLoc;
glm::mat4 projection;
GLuint projectionLoc;
glm::mat3 normalMatrix;
GLuint normalMatrixLoc;
glm::mat3 lightDirMatrix;
GLuint lightDirMatrixLoc;

//fog
int foginit = 0;
GLint foginitLoc;

//skybox
gps::SkyBox skyBox;
gps::Shader skyboxShader;
std::vector<const GLchar*> faces;

//light
glm::vec3 lightDir;
GLuint lightDirLoc;
glm::vec3 lightColor;
GLuint lightColorLoc;

glm::vec3 lightDir2;
GLuint lightDirLoc2;

//camera
gps::Camera myCamera(glm::vec3(0.0f, 1.0f, 2.5f), glm::vec3(0.0f, 0.0f, 0.0f));
GLfloat cameraSpeed = 0.1f;

//others
bool pressedKeys[1024];
GLfloat lightAngle;
GLfloat yaw = -90.0f;
GLfloat pitch = 0.0f;
GLfloat lastX = glWindowWidth / 2.0;
GLfloat lastY = glWindowHeight / 2.0;
bool first = true;

//models and shaders
gps::Model3D myModel;
gps::Model3D mainScene;
gps::Model3D lightCube;
gps::Shader customShader;
gps::Shader lightShader;
gps::Shader depthMapShader;
gps::Model3D lightPost;

//map
GLuint shadowMapFBO;
GLuint depthMapTexture;

//variables for mode movement
float up = -1.8f;
float forward = 5.8f;
float modelAngle = -90.0f;
float right = 7.0f;
int ok = 0;
int ok2 = -1;

//check errors
GLenum glCheckError_(const char *file, int line)
{
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR)
	{
		std::string error;
		switch (errorCode)
		{
		case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
		case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
		case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
		case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
		case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
		case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
		case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
		}
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}

//windows resize
void windowResizeCallback(GLFWwindow* window, int width, int height)
{
	fprintf(stdout, "window resized to width: %d , and height: %d\n", width, height);

	//for RETINA display
	glfwGetFramebufferSize(glWindow, &retina_width, &retina_height);

	customShader.useShaderProgram();

	//set projection matrix
	glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);

	//send matrix data to shader
	GLint projLoc = glGetUniformLocation(customShader.shaderProgram, "projection");
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
	
	lightShader.useShaderProgram();
	
	glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

	//set Viewport transform
	glViewport(0, 0, retina_width, retina_height);
}

//keyboard actions
void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	//close app
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, GL_TRUE);
	}

	//other keys
	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
		{
			pressedKeys[key] = true;
		}
		else if (action == GLFW_RELEASE)
		{
			pressedKeys[key] = false;
		}
	}
}

//mouse actions
void mouseCallback(GLFWwindow* window, double xpos, double ypos)
{
	if (first) 
	{
		lastX = (GLfloat)xpos;
		lastY = (GLfloat)ypos;
		first = false;
	}

	GLfloat xoffset = (GLfloat)xpos - lastX;
	GLfloat yoffset = lastY - (GLfloat)ypos;

	lastX = (GLfloat)xpos;
	lastY = (GLfloat)ypos;

	GLfloat sensitivity = 0.05f;

	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw += xoffset;
	pitch += yoffset;

	if (pitch > 89.0f)
	{
		pitch = 89.0f;
	}

	if (pitch < -89.0f)
	{
		pitch = -89.0f;
	}

	myCamera.rotate(pitch, yaw);
}

//camer animation
void rotateCamera(float angle) 
{
	if (cameraAnimation) 
	{
		cameraAngle -= 0.3f;
		myCamera.sceneVisualization(cameraAngle);
	}
}

//key bindings
void processMovement()
{
	//camera auto rotation
	if (pressedKeys[GLFW_KEY_C]) 
	{
		cameraAnimation = true;
	}

	if (pressedKeys[GLFW_KEY_V])
	{
		cameraAnimation = false;
	}

	//camera movement 
	if (pressedKeys[GLFW_KEY_W])
	{
		myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
	}

	if (pressedKeys[GLFW_KEY_S])
	{
		myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
	}

	if (pressedKeys[GLFW_KEY_A])
	{
		myCamera.move(gps::MOVE_LEFT, cameraSpeed);
	}

	if (pressedKeys[GLFW_KEY_D]) 
	{
		myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
	}

	//light movement
	if (pressedKeys[GLFW_KEY_J]) 
	{
		lightAngle += 0.3f;

		if (lightAngle > 360.0f)
		{
			lightAngle -= 360.0f;
		}

		glm::vec3 lightDirTr = glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(lightDir, 1.0f));
		customShader.useShaderProgram();
		glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDirTr));
	}

	if (pressedKeys[GLFW_KEY_L]) 
	{
		lightAngle -= 0.3f; 

		if (lightAngle < 0.0f)
		{
			lightAngle += 360.0f;
		}

		glm::vec3 lightDirTr = glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(lightDir, 1.0f));
		customShader.useShaderProgram();
		glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDirTr));
	}	

	//toggle wireframe view
	if (pressedKeys[GLFW_KEY_O]) 
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}

	//toggle point view
	if (pressedKeys[GLFW_KEY_P])
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
	}

	//toggle texture view
	if (pressedKeys[GLFW_KEY_I])
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	//toggle fog
	if (glfwGetKey(glWindow, GLFW_KEY_G) == GLFW_PRESS)
	{
		foginit = 0;

		//models fog
		customShader.useShaderProgram();
		foginitLoc = glGetUniformLocation(customShader.shaderProgram, "foginit");
		glUniform1i(foginitLoc, foginit);

		//skybox fog
		skyboxShader.useShaderProgram();
		foginitLoc = glGetUniformLocation(skyboxShader.shaderProgram, "foginit");
		glUniform1i(foginitLoc, foginit);
	}

	if (glfwGetKey(glWindow, GLFW_KEY_F) == GLFW_PRESS) 
	{
		foginit = 1;

		//models fog
		customShader.useShaderProgram();
		foginitLoc = glGetUniformLocation(customShader.shaderProgram, "foginit");
		glUniform1i(foginitLoc, foginit);

		//skybox fog
		skyboxShader.useShaderProgram();
		foginitLoc = glGetUniformLocation(skyboxShader.shaderProgram, "foginit");
		glUniform1i(foginitLoc, foginit);
	}

	//toggle model animation
	if (pressedKeys[GLFW_KEY_SPACE])
	{
		int aux = ok;
		ok = ok2;
		ok2 = aux;
	}

}

//initialize window
bool initOpenGLWindow()
{
	//check for error
	if (!glfwInit()) 
	{
		fprintf(stderr, "ERROR: could not start GLFW3\n");
		return false;
	}

	//for Mac OS X
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	//create window
	glWindow = glfwCreateWindow(glWindowWidth, glWindowHeight, "OpenGL Shader Example", NULL, NULL);
	if (!glWindow) 
	{
		fprintf(stderr, "ERROR: could not open window with GLFW3\n");
		glfwTerminate();
		return false;
	}

	glfwSetWindowSizeCallback(glWindow, windowResizeCallback);
	glfwMakeContextCurrent(glWindow);

	glfwWindowHint(GLFW_SAMPLES, 4);

	// start GLEW extension handler
	glewExperimental = GL_TRUE;
	glewInit();

	// get version info
	const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
	const GLubyte* version = glGetString(GL_VERSION); // version as a string
	printf("Renderer: %s\n", renderer);
	printf("OpenGL version supported %s\n", version);

	//for RETINA display
	glfwGetFramebufferSize(glWindow, &retina_width, &retina_height);

	//set key and mouse callbacks
	glfwSetKeyCallback(glWindow, keyboardCallback);
	glfwSetCursorPosCallback(glWindow, mouseCallback);

	return true;
}

//initializa state
void initOpenGLState()
{
	glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
	glViewport(0, 0, retina_width, retina_height);

	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"

	glCullFace(GL_BACK); // cull back face
	glFrontFace(GL_CCW); // GL_CCW for counter clock-wise

	glfwSetInputMode(glWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

//FBOs
void initFBOs()
{
	//generate FBO ID
	glGenFramebuffers(1, &shadowMapFBO);

	//create depth texture for FBO
	glGenTextures(1, &depthMapTexture);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	//attach texture to FBO
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

//compute light
glm::mat4 computeLightSpaceTrMatrix()
{
	const GLfloat near_plane = 1.0f, far_plane = 40.0f;

	glm::mat4 lightProjection = glm::ortho(-40.0f, 40.0f, -40.0f, 40.0f, near_plane, far_plane);
	glm::vec3 lightDirTr = glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(lightDir, 1.0f));
	glm::mat4 lightView = glm::lookAt(lightDirTr, myCamera.getCameraTarget(), glm::vec3(0.0f, 1.0f, 0.0f));

	return lightProjection * lightView;
}

//fetch models
void initModels()
{
	myModel = gps::Model3D("objects/monster/Lambent_Male.obj", "objects/monster/");
	mainScene = gps::Model3D("objects/FinalProject/final1.obj", "objects/FinalProject/");
	lightPost = gps::Model3D("objects/secondLight/tikiTorch.mtl.obj", "objects/secondLight/");
}

//fetch shaders
void initShaders()
{
	customShader.loadShader("shaders/shaderStart.vert", "shaders/shaderStart.frag");
	lightShader.loadShader("shaders/lightCube.vert", "shaders/lightCube.frag");
	depthMapShader.loadShader("shaders/simpleDepthMap.vert", "shaders/simpleDepthMap.frag");
	skyboxShader.loadShader("shaders/skyboxShader.vert", "shaders/skyboxShader.frag");
}

//initialize uniforms
void initUniforms()
{
	customShader.useShaderProgram();

	modelLoc = glGetUniformLocation(customShader.shaderProgram, "model");
	viewLoc = glGetUniformLocation(customShader.shaderProgram, "view");
	normalMatrixLoc = glGetUniformLocation(customShader.shaderProgram, "normalMatrix");
	lightDirMatrixLoc = glGetUniformLocation(customShader.shaderProgram, "lightDirMatrix");

	projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
	projectionLoc = glGetUniformLocation(customShader.shaderProgram, "projection");
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));	

	//set the light direction (direction towards the light)
	lightDir = glm::vec3(0.0f, 10.0f, 10.0f);
	lightDirLoc = glGetUniformLocation(customShader.shaderProgram, "lightDir");
	glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDir));

	lightDir2 = glm::vec3(-1.0f, 0.0f, 1.5f);
	lightDirLoc2 = glGetUniformLocation(customShader.shaderProgram, "lightDir2");
	glUniform3fv(lightDirLoc2, 1, glm::value_ptr(lightDir2));

	//set light color
	lightColor = glm::vec3(1.0f, 1.0f, 1.0f); //white light
	lightColorLoc = glGetUniformLocation(customShader.shaderProgram, "lightColor");
	glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

	lightShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

	//Skybox
	faces.push_back("skybox/right.tga");
	faces.push_back("skybox/left.tga");
	faces.push_back("skybox/top.tga");
	faces.push_back("skybox/bottom.tga");
	faces.push_back("skybox/back.tga");
	faces.push_back("skybox/front.tga");

	skyBox.Load(faces);

	skyboxShader.useShaderProgram();

	//camera
	view = myCamera.getViewMatrix();
	glUniformMatrix4fv(glGetUniformLocation(skyboxShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));

	projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
	glUniformMatrix4fv(glGetUniformLocation(skyboxShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
}

//how the model moves
void moveModel()
{
	//up->forward->turn->right
	if (ok == 0)
	{
		if (up <= -1.0f)
		{
			up += 0.005;
		}
		else if (forward > 0.0f)
		{
			forward -= 0.01f;
		}
		else if (modelAngle <= 0.0f)
		{
			modelAngle += 0.6;
		}
		else if (right <= 7.5)
		{
			right += 0.01;
		}
		else ok = 1;
	}

	//turn->backwards->down
	if (ok == 1)
	{
		if (modelAngle <= 90.0f)
		{
			modelAngle += 0.6;
		}
		else if (forward < 6.0)
		{
			forward += 0.01;
		}
		else if (up >= -1.8)
		{
			up -= 0.005;
		}
		else
		{
			up = -1.8f;
			forward = 5.8f;
			modelAngle = -90.0f;
			right = 7.0f;
			ok = 0;
		}
	}
}

//render the scene
void renderScene()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	processMovement();	

	//render the scene to the depth buffer (first pass)
	depthMapShader.useShaderProgram();

	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceTrMatrix"),
		1,
		GL_FALSE,
		glm::value_ptr(computeLightSpaceTrMatrix()));
		
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	
	//create model matrix for nanosuit
	model = glm::mat4(1.0f);
	model = glm::translate(glm::vec3(forward, up, right));
	model = glm::rotate(model, glm::radians(modelAngle), glm::vec3(0, 1, 0));
	model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.2f));

	//send model matrix to shader
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"),
		1,
		GL_FALSE,
		glm::value_ptr(model));

	myModel.Draw(depthMapShader);

	//create model matrix for mainScene
	model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f));

	//send model matrix to shader
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"), 
						1, 
						GL_FALSE, 
						glm::value_ptr(model));

	mainScene.Draw(depthMapShader);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//render the scene (second pass)
	customShader.useShaderProgram();

	//send lightSpace matrix to shader
	glUniformMatrix4fv(glGetUniformLocation(customShader.shaderProgram, "lightSpaceTrMatrix"),
		1,
		GL_FALSE,
		glm::value_ptr(computeLightSpaceTrMatrix()));

	//send view matrix to shader
	view = myCamera.getViewMatrix();
	glUniformMatrix4fv(glGetUniformLocation(customShader.shaderProgram, "view"),
		1,
		GL_FALSE,
		glm::value_ptr(view));	

	//compute light direction transformation matrix
	lightDirMatrix = glm::mat3(glm::inverseTranspose(view));

	//send lightDir matrix data to shader
	glUniformMatrix3fv(lightDirMatrixLoc, 1, GL_FALSE, glm::value_ptr(lightDirMatrix));
	glViewport(0, 0, retina_width, retina_height);
	customShader.useShaderProgram();

	//bind the depth map
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
	glUniform1i(glGetUniformLocation(customShader.shaderProgram, "shadowMap"), 3);
	
	//create model matrix for nanosuit
	model = glm::mat4(1.0f);
	model = glm::translate(glm::vec3(forward, up, right));
	model = glm::rotate(model, glm::radians(modelAngle), glm::vec3(0, 1, 0));
	model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.2f));

	//send model matrix data to shader	
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	//compute normal matrix
	normalMatrix = glm::mat3(glm::inverseTranspose(view*model));

	//send normal matrix data to shader
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	moveModel();
	myModel.Draw(customShader);
		
	//create model matrix for mainScene
	model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
	//send model matrix data to shader
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	//create normal matrix
	normalMatrix = glm::mat3(glm::inverseTranspose(view*model));

	//send normal matrix data to shader
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	mainScene.Draw(customShader);

	//draw skybox
	skyBox.Draw(skyboxShader, view, projection);

	//camera animation
	currentTime = glfwGetTime();
	rotateCamera(currentTime);

	//light lightPost
	lightShader.useShaderProgram();

	model = glm::translate(glm::mat4(1.0f), glm::vec3(-2.0, -1.0, 25.0));
	model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));
	glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	glUniform3fv(glGetUniformLocation(lightShader.shaderProgram, "vecColor"), 1, glm::value_ptr(glm::vec3(1.0, 1.0, 0.0)));

	lightPost.Draw(lightShader);
}

//main function
int main(int argc, const char * argv[]) 
{
	//initialization
	initOpenGLWindow();
	initOpenGLState();
	initFBOs();
	initModels();
	initShaders();
	initUniforms();	

	//check errors
	glCheckError();

	//render loop
	while (!glfwWindowShouldClose(glWindow)) 
	{
		renderScene();

		glfwPollEvents();
		glfwSwapBuffers(glWindow);
	}

	//close GL context and any other GLFW resources
	glfwTerminate();

	return 0;
}