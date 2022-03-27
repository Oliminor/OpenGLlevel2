#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
#include "GL/glew.h"
#include "GL/3dgl.h"
#include "GL/glut.h"
#include "GL/freeglut_ext.h"

// Include GLM core features
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#pragma comment (lib, "glew32.lib")

using namespace std;
using namespace _3dgl;
using namespace glm;

// GLSL Program
C3dglProgram Program;
C3dglProgram ProgramEffect;
C3dglProgram ProgramWater;
C3dglProgram ProgramTerrain;
C3dglProgram ProgramParticle;

// Post Process
GLuint WImage = 800, HImage = 600;
GLuint TempWImage = 0, TempHImage = 0;
GLuint bufQuad;

// Skybox
C3dglSkyBox skybox;

// Water specific variables
float waterLevel = 11.0f;

// Terrain
C3dglTerrain terrain, water;

// quad size
float quadSize = 4;

// Cube map
GLuint idTexCube;
GLuint idTexCube2;
GLuint idTexCube3;

// Shadow Map
GLuint idTexShadowMap;

// Particle System Params
const float PERIOD = 0.01f;
const float LIFETIME = 20;
const int NPARTICLES = (int)(LIFETIME / PERIOD) * 10;
GLuint idBufferVelocity;
GLuint idBufferStartTime;
GLuint idBufferInitialPos;
GLuint idTexParticle;


// 3D models
C3dglModel delorean;
C3dglModel deloreanWheel;
C3dglModel SFCube;
C3dglModel ring;
C3dglModel character;
C3dglModel character2;
C3dglModel character3;
C3dglModel sword;
C3dglModel radio;
C3dglModel scout;

GLuint idTexNone;
GLuint idTexSandC;
GLuint idTexSandN;
GLuint idTexStoneS;
GLuint idTexStoneB;
GLuint idRoadN;
GLuint idCharacter;
GLuint idCharacterN;
GLuint idSword;

GLuint idTexScreen;

float rotateSpeed = 60;
float transition;
vec3 finalFogColor;
int postProcessMode = 0;
int animationMode = 0;

// buffers names
unsigned vertexBuffer = 0;
unsigned normalBuffer = 0;
unsigned indexBuffer = 0;
unsigned idFBO;
unsigned idFBO2;

// camera position (for first person type camera navigation)
mat4 matrixView;			// The View Matrix
float angleTilt = 15;		// Tilt Angle
float angleRot = 0.1f;		// Camera orbiting angle
vec3 cam(0);				// Camera movement values

void RefreshPostProcessing ()
{
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	WImage = viewport[2];
	HImage = viewport[3];

	if (TempWImage == WImage && TempHImage == HImage) return;
	ProgramEffect.SendUniform("resolution", WImage, HImage);

	// Create screen space texture
	glGenTextures(1, &idTexScreen);
	glBindTexture(GL_TEXTURE_2D, idTexScreen);

	// Texture parameters - to get nice filtering 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// This will allocate an uninitilised texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WImage, HImage, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	// Create a framebuffer object (FBO)
	glGenFramebuffers(1, &idFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, idFBO);

	// Attach a depth buffer
	GLuint depth_rb;
	glGenRenderbuffers(1, &depth_rb);
	glBindRenderbuffer(GL_RENDERBUFFER, depth_rb);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, WImage, HImage);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rb);

	// attach the texture to FBO colour attachment point
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, idTexScreen, 0);

	// switch back to window-system-provided framebuffer
	glBindFramebufferEXT(GL_FRAMEBUFFER, 0);

	// Create Quad
	float vertices[] = {
		0.0f, 0.0f, 0.0f,	0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,	1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,	1.0f, 1.0f,
		0.0f, 1.0f, 0.0f,	0.0f, 1.0f
	};

	// Generate the buffer name
	glGenBuffers(1, &bufQuad);
	// Bind the vertex buffer and send data
	glBindBuffer(GL_ARRAY_BUFFER, bufQuad);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	TempWImage = WImage;
	TempHImage = HImage;
}

bool init()
{
	// rendering states
	glEnable(GL_DEPTH_TEST);	// depth test is necessary for most 3D scenes
	glEnable(GL_NORMALIZE);		// normalization is needed by AssImp library models
	glShadeModel(GL_SMOOTH);	// smooth shading mode is the default one; try GL_FLAT here!
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);	// this is the default one; try GL_LINE!

#pragma region // Initialise Shaders

	// Initialise Shaders
	C3dglShader VertexShader;
	C3dglShader FragmentShader;

	if (!VertexShader.Create(GL_VERTEX_SHADER)) return false;
	if (!VertexShader.LoadFromFile("shaders/basic.vert")) return false;
	if (!VertexShader.Compile()) return false;

	if (!FragmentShader.Create(GL_FRAGMENT_SHADER)) return false;
	if (!FragmentShader.LoadFromFile("shaders/basic.frag")) return false;
	if (!FragmentShader.Compile()) return false;

	if (!Program.Create()) return false;
	if (!Program.Attach(VertexShader)) return false;
	if (!Program.Attach(FragmentShader)) return false;
	if (!Program.Link()) return false;
	if (!Program.Use(true)) return false;

	if (!VertexShader.Create(GL_VERTEX_SHADER)) return false;
	if (!VertexShader.LoadFromFile("shaders/effect.vert")) return false;
	if (!VertexShader.Compile()) return false;

	if (!FragmentShader.Create(GL_FRAGMENT_SHADER)) return false;
	if (!FragmentShader.LoadFromFile("shaders/effect.frag")) return false;
	if (!FragmentShader.Compile()) return false;

	if (!ProgramEffect.Create()) return false;
	if (!ProgramEffect.Attach(VertexShader)) return false;
	if (!ProgramEffect.Attach(FragmentShader)) return false;
	if (!ProgramEffect.Link()) return false;
	if (!ProgramEffect.Use(true)) return false;

	if (!VertexShader.Create(GL_VERTEX_SHADER)) return false;
	if (!VertexShader.LoadFromFile("shaders/water.vert")) return false;
	if (!VertexShader.Compile()) return false;

	if (!FragmentShader.Create(GL_FRAGMENT_SHADER)) return false;
	if (!FragmentShader.LoadFromFile("shaders/water.frag")) return false;
	if (!FragmentShader.Compile()) return false;

	if (!ProgramWater.Create()) return false;
	if (!ProgramWater.Attach(VertexShader)) return false;
	if (!ProgramWater.Attach(FragmentShader)) return false;
	if (!ProgramWater.Link()) return false;
	if (!ProgramWater.Use(true)) return false;

	if (!VertexShader.Create(GL_VERTEX_SHADER)) return false;
	if (!VertexShader.LoadFromFile("shaders/terrain.vert")) return false;
	if (!VertexShader.Compile()) return false;

	if (!FragmentShader.Create(GL_FRAGMENT_SHADER)) return false;
	if (!FragmentShader.LoadFromFile("shaders/terrain.frag")) return false;
	if (!FragmentShader.Compile()) return false;

	if (!ProgramTerrain.Create()) return false;
	if (!ProgramTerrain.Attach(VertexShader)) return false;
	if (!ProgramTerrain.Attach(FragmentShader)) return false;
	if (!ProgramTerrain.Link()) return false;
	if (!ProgramTerrain.Use(true)) return false;

	if (!VertexShader.Create(GL_VERTEX_SHADER)) return false;
	if (!VertexShader.LoadFromFile("shaders/particle.vert")) return false;
	if (!VertexShader.Compile()) return false;

	if (!FragmentShader.Create(GL_FRAGMENT_SHADER)) return false;
	if (!FragmentShader.LoadFromFile("shaders/particle.frag")) return false;
	if (!FragmentShader.Compile()) return false;

	if (!ProgramParticle.Create()) return false;
	if (!ProgramParticle.Attach(VertexShader)) return false;
	if (!ProgramParticle.Attach(FragmentShader)) return false;
	if (!ProgramParticle.Link()) return false;
	if (!ProgramParticle.Use(true)) return false;

	Program.Use();

#pragma endregion

	glutSetVertexAttribCoord3(Program.GetAttribLocation("aVertex"));
	glutSetVertexAttribNormal(Program.GetAttribLocation("aNormal"));

	// load your 3D models here!
	if (!delorean.load("models\\ship\\delorean.obj")) return false;
	delorean.loadMaterials("models\\ship\\delorean.mtl");
	delorean.getMaterial(7)->loadTexture(GL_TEXTURE0, "models/ship", "delorean.jpg");
	if (!deloreanWheel.load("models\\ship\\deloranWheel.obj")) return false;

	if (!SFCube.load("models\\SFCube\\cube.obj")) return false;
	if (!ring.load("models\\bg\\ring.obj")) return false;
	if (!character.load("models\\character\\sitIdle.dae")) return false;
	if (!character2.load("models\\character\\sitIdle2.dae")) return false;
	if (!character3.load("models\\character\\punchingBag.dae")) return false;
	if (!sword.load("models\\sword\\sword.obj")) return false;
	if (!scout.load("models\\scout.obj")) return false;
	if (!radio.load("models\\radio\\Radio.obj")) return false;
	radio.loadMaterials("models\\radio\\Radio.mtl");
	radio.getMaterial(0)->loadTexture(GL_TEXTURE0, "models/radio", "TextureRadio.png");

	character.loadAnimations();
	character2.loadAnimations();
	character3.loadAnimations();

	// load Sky Box     
	if (!skybox.load("models\\skybox\\right.png", "models\\skybox\\left.png", "models\\skybox\\middle.png",
		"models\\skybox\\middle2.png", "models\\skybox\\top.png", "models\\skybox\\bottom.png")) return false;

	// Terrain map load
	if (!terrain.loadHeightmap("models\\sand.bmp", 75)) return false;
	if (!water.loadHeightmap("models\\watermap.png", 10)) return false;

#pragma region // Load Textures

	// Textures
	C3dglBitmap bm;

	// none (simple-white) texture
	glGenTextures(1, &idTexNone);
	glBindTexture(GL_TEXTURE_2D, idTexNone);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	BYTE bytes[] = { 255, 255, 255 };
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_BGR, GL_UNSIGNED_BYTE, &bytes);

	// Stone Texture
	bm.Load("models/sandC.jpg", GL_RGBA);
	if (!bm.GetBits()) return false;

	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &idTexSandC);
	glBindTexture(GL_TEXTURE_2D, idTexSandC);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bm.GetWidth(), bm.GetHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, bm.GetBits());

	//Stone Normal Texture
	bm.Load("models/sandN.jpg", GL_RGBA);
	if (!bm.GetBits()) return false;

	glActiveTexture(GL_TEXTURE1);
	glGenTextures(1, &idTexSandN);
	glBindTexture(GL_TEXTURE_2D, idTexSandN);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bm.GetWidth(), bm.GetHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, bm.GetBits());

	// Water Shore and ShoreBed
	// Stone Texture - Shore
	bm.Load("models/rockTexture.jpg", GL_RGBA);
	if (!bm.GetBits()) return false;

	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &idTexStoneB);
	glBindTexture(GL_TEXTURE_2D, idTexStoneB);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bm.GetWidth(), bm.GetHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, bm.GetBits());

	// Stone Texture - ShoreBed
	bm.Load("models/rockTextureR.jpg", GL_RGBA);
	if (!bm.GetBits()) return false;

	glActiveTexture(GL_TEXTURE1);
	glGenTextures(1, &idTexStoneS);
	glBindTexture(GL_TEXTURE_2D, idTexStoneS);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bm.GetWidth(), bm.GetHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, bm.GetBits());

	// Smoke Particle
	bm.Load("models/smoke.png", GL_RGBA);
	if (!bm.GetBits()) return false;

	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &idTexParticle);
	glBindTexture(GL_TEXTURE_2D, idTexParticle);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bm.GetWidth(), bm.GetHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, bm.GetBits());

	// Character
	bm.Load("models/character/characterColor.png", GL_RGBA);
	if (!bm.GetBits()) return false;

	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &idCharacter);
	glBindTexture(GL_TEXTURE_2D, idCharacter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bm.GetWidth(), bm.GetHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, bm.GetBits());

	// Character Normal Texture
	bm.Load("models/character/characterNormal.png", GL_RGBA);
	if (!bm.GetBits()) return false;

	glActiveTexture(GL_TEXTURE1);
	glGenTextures(1, &idCharacterN);
	glBindTexture(GL_TEXTURE_2D, idCharacterN);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bm.GetWidth(), bm.GetHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, bm.GetBits());

	// Sword
	bm.Load("models/sword/sword.png", GL_RGBA);
	if (!bm.GetBits()) return false;

	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &idSword);
	glBindTexture(GL_TEXTURE_2D, idSword);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bm.GetWidth(), bm.GetHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, bm.GetBits());

#pragma endregion

#pragma region // Post Process

	RefreshPostProcessing();

#pragma endregion

#pragma region // Static Cube map;

	// load Static Cube Map
	glActiveTexture(GL_TEXTURE2);
	glGenTextures(1, &idTexCube);
	glBindTexture(GL_TEXTURE_CUBE_MAP, idTexCube);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	bm.Load("models\\cube\\middle2.png", GL_RGBA); glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0,
		GL_RGBA, bm.GetWidth(), abs(bm.GetHeight()), 0, GL_RGBA, GL_UNSIGNED_BYTE, bm.GetBits());
	bm.Load("models\\cube\\left.png", GL_RGBA); glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0,
		GL_RGBA, bm.GetWidth(), abs(bm.GetHeight()), 0, GL_RGBA, GL_UNSIGNED_BYTE, bm.GetBits());
	bm.Load("models\\cube\\middle.png", GL_RGBA); glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0,
		GL_RGBA, bm.GetWidth(), abs(bm.GetHeight()), 0, GL_RGBA, GL_UNSIGNED_BYTE, bm.GetBits());
	bm.Load("models\\cube\\right.png", GL_RGBA); glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0,
		GL_RGBA, bm.GetWidth(), abs(bm.GetHeight()), 0, GL_RGBA, GL_UNSIGNED_BYTE, bm.GetBits());
	bm.Load("models\\cube\\top.png", GL_RGBA); glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0,
		GL_RGBA, bm.GetWidth(), abs(bm.GetHeight()), 0, GL_RGBA, GL_UNSIGNED_BYTE, bm.GetBits());
	bm.Load("models\\cube\\bottom.png", GL_RGBA); glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0,
		GL_RGBA, bm.GetWidth(), abs(bm.GetHeight()), 0, GL_RGBA, GL_UNSIGNED_BYTE, bm.GetBits());

#pragma endregion

#pragma region // Dynamic Cube map;
	
	// load Static Cube Map
	glGenTextures(1, &idTexCube2);
	glBindTexture(GL_TEXTURE_CUBE_MAP, idTexCube2);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	glGenTextures(1, &idTexCube3);
	glBindTexture(GL_TEXTURE_CUBE_MAP, idTexCube3);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	
#pragma endregion

#pragma region // Shadow map;

	// Create shadow map texture
	glActiveTexture(GL_TEXTURE7);
	glGenTextures(1, &idTexShadowMap);
	glBindTexture(GL_TEXTURE_2D, idTexShadowMap);

	// Create a framebuffer object (FBO)
	glGenFramebuffers(1, &idFBO2);
	glBindFramebuffer(GL_FRAMEBUFFER_EXT, idFBO2);

	// Instruct openGL that we won't bind a color texture with the currently binded FBO
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	// attach the texture to FBO depth attachment point
	glFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, idTexShadowMap, 0);

	// switch back to window-system-provided framebuffer
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	// Texture parameters - to get nice filtering & avoid artefact on the edges of the shadowmap
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LESS);

	// This will associate the texture with the depth component in the Z-buffer
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
		WImage * 2, HImage * 2, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);

	// revert to texture unit 0
	glActiveTexture(GL_TEXTURE0);

#pragma endregion

#pragma region // Particle System

	// Setup the particle system
	ProgramParticle.SendUniform("gravity", 0.0, -10.0, 0.0);
	ProgramParticle.SendUniform("particleLifetime", LIFETIME);

	// Prepare the particle buffers
	std::vector<float> bufferVelocity;
	std::vector<float> bufferStartTime;
	std::vector<float> bufferInitialPos;
	float time = 0;
	for (int i = 0; i < NPARTICLES; i++)
	{
		float theta = (float)M_PI / 6.f * (float)rand() / (float)RAND_MAX;
		float phi = (float)M_PI * 2.f * (float)rand() / (float)RAND_MAX;
		float x = sin(theta) * cos(phi);
		float y = cos(theta);
		float z = sin(theta) * sin(phi);
		float v = 2 + 0.5f * (float)rand() / (float)RAND_MAX;

		bufferVelocity.push_back(x * v);
		bufferVelocity.push_back(y * v);
		bufferVelocity.push_back(z * v);

		bufferStartTime.push_back(time);

		double randomPos = (20000 - rand() % 40000) / (double)100;
		double randomPos2 = (20000 - rand() % 40000) / (double)100;
		bufferInitialPos.push_back(randomPos);
		bufferInitialPos.push_back(0);
		bufferInitialPos.push_back(randomPos2);
		time += PERIOD;
	}
	glGenBuffers(1, &idBufferVelocity);
	glBindBuffer(GL_ARRAY_BUFFER, idBufferVelocity);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * bufferVelocity.size(), &bufferVelocity[0],
		GL_STATIC_DRAW);
	glGenBuffers(1, &idBufferStartTime);
	glBindBuffer(GL_ARRAY_BUFFER, idBufferStartTime);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * bufferStartTime.size(), &bufferStartTime[0],
		GL_STATIC_DRAW);
	glGenBuffers(1, &idBufferInitialPos);
	glBindBuffer(GL_ARRAY_BUFFER, idBufferInitialPos);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * bufferInitialPos.size(), &bufferInitialPos[0],
		GL_STATIC_DRAW);

	// setup the point size for particle
	glEnable(0x8642);  
	glEnable(GL_POINT_SPRITE);

#pragma endregion

	// Water alpha blend
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Initialise the View Matrix (initial position of the camera)
	matrixView = rotate(mat4(1.f), radians(angleTilt), vec3(1.f, 0.f, 0.f));
	matrixView *= lookAt(
		vec3(0.0, 5.0, 25.0),
		vec3(0.0, 5.0, 0.0),
		vec3(0.0, 1.0, 0.0));

	// setup the screen background colour
	glClearColor(0.4f, 0.3f, 0.1f, 1.0f);   // deep grey background

	// Texture
	ProgramParticle.SendUniform("texture0", 0);
	ProgramEffect.SendUniform("texture0", 0);
	ProgramEffect.SendUniform("mode", 0);
	ProgramTerrain.SendUniform("texture0", 0);
	ProgramTerrain.SendUniform("textureBed", 0);
	ProgramTerrain.SendUniform("textureShore", 1);
	Program.SendUniform("texture0", 0);
	Program.SendUniform("textureNormal", 1);
	Program.SendUniform("textureCubeMap", 2);
	Program.SendUniform("textureCubeMap2", 2);
	Program.SendUniform("textureCubeMap3", 2);
	Program.SendUniform("shadowMap", 7);
	ProgramTerrain.SendUniform("shadowMap", 7);
	ProgramTerrain.SendUniform("textureNormal", 1);

	// Post process
	ProgramEffect.SendUniform("contrast", 1.2);
	ProgramEffect.SendUniform("brightness", 0.8);

	// Fog settings
	ProgramTerrain.SendUniform("fogDensity2", 0.02);
	ProgramTerrain.SendUniform("fogColour", 0.2f, 0.22f, 0.02f);
	ProgramTerrain.SendUniform("fogColour2", 0.0f, 0.14f, 0.31f);

	Program.SendUniform("fogDensity", 0.05);
	Program.SendUniform("fogColour", 0.0f, 0.14f, 0.31f);

	// Opacity Settings
	Program.SendUniform("opacity", 1.0f);

	// Water settings
	ProgramWater.SendUniform("waterColor", 0.2f, 0.22f, 0.02f);
	ProgramWater.SendUniform("skyColor", 0.2f, 0.6f, 1.f);

	ProgramTerrain.SendUniform("waterLevel", waterLevel);

	// Ambient Light
	Program.SendUniform("lightAmbient.on", 1);
	Program.SendUniform("lightAmbient.color", 0.5, 0.5, 0.5);
	Program.SendUniform("materialAmbient", 0.2, 0.2, 0.2);
	ProgramTerrain.SendUniform("lightAmbient.on", 1);
	ProgramTerrain.SendUniform("lightAmbient.color", 0.5, 0.5, 0.5);
	ProgramTerrain.SendUniform("materialAmbient", 0.2, 0.2, 0.2);

	// Direction Light
	Program.SendUniform("lightDir.on", 1);
	Program.SendUniform("lightDir.direction", 0.0, 1.0, 0.0);
	Program.SendUniform("lightDir.diffuse", 1.5, 1.5, 1.5);	
	ProgramTerrain.SendUniform("lightDir.on", 1);
	ProgramTerrain.SendUniform("lightDir.direction", 0.0, 1.0, 0.0);
	ProgramTerrain.SendUniform("lightDir.diffuse", 1.5, 1.5, 1.5);	  

#pragma region // Point Light

	Program.SendUniform("lightPoint[0].on", 1);
	Program.SendUniform("lightPoint[0].att_quadratic", 0.005);
	Program.SendUniform("lightPoint[0].position", 0.0f, 0.0f, 0.0f);
	Program.SendUniform("lightPoint[0].diffuse", 0.1, 0.1, 0.1);
	Program.SendUniform("lightPoint[0].specular", 0.01, 0.01, 0.01);

	ProgramTerrain.SendUniform("lightPoint[0].on", 1);
	ProgramTerrain.SendUniform("lightPoint[0].att_quadratic", 0.005);
	ProgramTerrain.SendUniform("lightPoint[0].position", 0.0f, 0.0f, 0.0f);
	ProgramTerrain.SendUniform("lightPoint[0].diffuse", 0.1, 0.1, 0.1);
	ProgramTerrain.SendUniform("lightPoint[0].specular", 0.01, 0.01, 0.01);

	Program.SendUniform("lightPoint[1].on", 1);
	Program.SendUniform("lightPoint[1].att_quadratic", 0.5);
	Program.SendUniform("lightPoint[1].position", 0.0f, 0.0f, 0.0f);
	Program.SendUniform("lightPoint[1].diffuse", 1.0, 0.2, 0.1);
	Program.SendUniform("lightPoint[1].specular", 0.01, 0.01, 0.01);

#pragma endregion

#pragma region // Spot lights

	// Spot light Blue 
	Program.SendUniform("lightSpot[0].on", 1);
	Program.SendUniform("lightSpot[0].position", 0.0f, 0.0f, 0.0f);
	Program.SendUniform("lightSpot[0].diffuse", 0.0, 3.0, 10.0);
	Program.SendUniform("lightSpot[0].specular", 2.0, 2.0, 2.0);
	Program.SendUniform("lightSpot[0].direction", -1.0, -1.0, 0.0);
	Program.SendUniform("lightSpot[0].cutoff", 60);
	Program.SendUniform("lightSpot[0].attenuation", 10.0);
	Program.SendUniform("lightSpot[0].att_quadratic", 0.01);

	ProgramTerrain.SendUniform("lightSpot[0].on", 1);
	ProgramTerrain.SendUniform("lightSpot[0].position", 0.0f, 0.0f, 0.0f);
	ProgramTerrain.SendUniform("lightSpot[0].diffuse", 0.0, 1.5, 10.0);
	ProgramTerrain.SendUniform("lightSpot[0].specular", 2.0, 2.0, 2.0);
	ProgramTerrain.SendUniform("lightSpot[0].direction", -1.0, -1.0, 0.0);
	ProgramTerrain.SendUniform("lightSpot[0].cutoff", 60);
	ProgramTerrain.SendUniform("lightSpot[0].attenuation", 10.0);
	ProgramTerrain.SendUniform("lightSpot[0].att_quadratic", 0.01);

	// Spot light Scout
	Program.SendUniform("lightSpot[1].on", 1);
	Program.SendUniform("lightSpot[1].diffuse", 5.0, 5.0, 5.0);
	Program.SendUniform("lightSpot[1].specular", 2.0, 2.0, 2.0);
	Program.SendUniform("lightSpot[1].direction", 0.0, -1.0, 0.0);
	Program.SendUniform("lightSpot[1].cutoff", 60);
	Program.SendUniform("lightSpot[1].attenuation", 10.0);
	Program.SendUniform("lightSpot[1].att_quadratic", 0.00005);

	ProgramTerrain.SendUniform("lightSpot[1].on", 1);
	ProgramTerrain.SendUniform("lightSpot[1].diffuse", 5.0, 5.0, 5.0);
	ProgramTerrain.SendUniform("lightSpot[1].specular", 2.0, 2.0, 2.0);
	ProgramTerrain.SendUniform("lightSpot[1].direction", 0.0, -1.0, 0.0);
	ProgramTerrain.SendUniform("lightSpot[1].cutoff", 60);
	ProgramTerrain.SendUniform("lightSpot[1].attenuation", 10.0);
	ProgramTerrain.SendUniform("lightSpot[1].att_quadratic", 0.00005);

	
#pragma endregion

	cout << endl;
	cout << "Use:" << endl;
	cout << "  WASD or arrow key to navigate" << endl;
	cout << "  QE or PgUp/Dn to move the camera up and down" << endl;
	cout << "  Shift+AD or arrow key to auto-orbit" << endl;
	cout << "  Drag the mouse to look around" << endl;
	cout << endl;

	return true;
}

void done()
{
}

void renderScene(mat4 &matrixView, float time, bool isLightOn)
{
	// Camera position  (Inverse Matrix Extraction)
	// https://community.khronos.org/t/extracting-camera-position-from-a-modelview-matrix/68031

	// mat4 viewModel = inverse(matrixView);
	// vec3 camPos(viewModel[3]);

	ProgramEffect.SendUniform("mode", postProcessMode);

	mat4 m;
	mat4 tempM;

	float speed = 20;
	float counter = (float)(GetTickCount() % 86400 / speed);
	float hour = counter / 3600 * speed;

	float step = hour * 15;

	transition = hour / 4;

	if (transition > 1) transition = 1;

	if (hour > 12) transition = (16 - hour) / 4;

	if (transition < 0) transition = 0;

	vec3 fogColorA = vec3(0.0f, 0.14f, 0.31f);
	vec3 fogColorB = vec3(0.4f, 0.3f, 0.1f);

	finalFogColor = fogColorA * (1 - transition) + fogColorB * transition;

#pragma region // Skybox
	Program.SendUniform("scaleX", 1);
	Program.SendUniform("scaleY", 1);
	Program.SendUniform("materialSpecular", 0.0, 0.0, 0.0);
	Program.SendUniform("lightSpot[0].on", 0);
	Program.SendUniform("lightSpot[1].on", 0);
	Program.SendUniform("lightSpot[2].on", 0);
	Program.SendUniform("lightSpot[3].on", 0);
	Program.SendUniform("lightPoint[1].on", 0);
	Program.SendUniform("lightSpot[1].on", 0);
	Program.SendUniform("lightDir.on", 0);
	Program.SendUniform("useShadowMap", 0);
	Program.SendUniform("useNormalMap", 0);
	Program.SendUniform("lightAmbient.color", 1.0, 1.0, 1.0);
	Program.SendUniform("materialAmbient", 1.5, 1.5, 1.5);
	Program.SendUniform("materialDiffuse", 0.3, 0.3, 0.3);
	Program.SendUniform("opacity", 1 - transition);

	m = matrixView;
	m = rotate(m, radians(180.f), vec3(0.0f, 1.0f, 0.0f));
	m = rotate(m, radians(step), vec3(1.0f, 0.0f, 0.0f));
	tempM = m;
	skybox.render(m);

	tempM = rotate(tempM, radians(230.f), vec3(1.0f, 0.0f, 0.0f));
	Program.SendUniform("lightDir.matrix", tempM);
	ProgramTerrain.SendUniform("lightDir.matrix", tempM);

	//Program.SendUniform("useShadowMap", 1);
	//ProgramTerrain.SendUniform("useShadowMap", 1);
	Program.SendUniform("lightDir.on", 1);
	Program.SendUniform("opacity", 1);
	Program.SendUniform("lightPoint[1].on", 1);
#pragma endregion

#pragma region // Deloran

	Program.SendUniform("fogColour", finalFogColor[0], finalFogColor[1], finalFogColor[2]);

	Program.SendUniform("fogDensity", 0.05);
	Program.SendUniform("materialAmbient", 0.1, 0.1, 0.1);
	Program.SendUniform("lightDir.diffuse",8.0, 8.0, 8.0);
	Program.SendUniform("materialDiffuse", 0.2f, 0.2f, 0.2f);

	m = matrixView;
	m = translate(m, vec3(55.0f, 18.0f, -5.0f));
	m = rotate(m, radians(110.0f), vec3(-0.1f, 1.0f, 0.0f));
	tempM = m;
	m = scale(m, vec3(5.0f, 5.0f, 5.0f));

	if ((int)step % 10 == 0 || (int)step % 12 == 0)
	{
		Program.SendUniform("reflectionPower", 0);
		Program.SendUniform("materialAmbient", 10.0, 10.0, 10.0);
		Program.SendUniform("lightAmbient.color", 0.0, 0.2, 0.8);
	}

	m = scale(m, vec3(1.001f, 1.001f, 1.001f));
	if (isLightOn) deloreanWheel.render(m);

	if (isLightOn)
	{
		tempM = translate(m, vec3(-1.0f, 1.0, -0.4f));
		Program.SendUniform("lightSpot[0].matrix", tempM);
		ProgramTerrain.SendUniform("lightSpot[0].matrix", tempM);
		if ((int)step % 10 == 0 || (int)step % 12 == 0)
		{
			Program.SendUniform("lightSpot[0].on", 1);
			ProgramTerrain.SendUniform("lightSpot[0].on", 1);
		}
		else
		{
			Program.SendUniform("lightSpot[0].on", 0);
			ProgramTerrain.SendUniform("lightSpot[0].on", 0);
		}
	}

	Program.SendUniform("useCubeMap", 0);
	glActiveTexture(GL_TEXTURE0);
	Program.SendUniform("reflectionPower", 0.0);


#pragma endregion

#pragma region // Sword

	Program.SendUniform("lightAmbient.color", 1.0, 1.0, 1.0);
	Program.SendUniform("materialAmbient", 0.1, 0.1, 0.1);
	Program.SendUniform("lightDir.diffuse", 5.0, 5.0, 5.0);
	Program.SendUniform("materialDiffuse", 0.2f, 0.2f, 0.2f);
	Program.SendUniform("shininess", 3.0f);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, idSword);

	m = matrixView;
	m = translate(m, vec3(56.0f, 19.0f, -11.0f));
	m = rotate(m, radians(160.0f), vec3(1.0f, 0.0f, 0.5f));
	m = scale(m, vec3(0.03f, 0.03f, 0.03f));
	sword.render(m);

	Program.SendUniform("shininess", 0.0f);
	glBindTexture(GL_TEXTURE_2D, idTexNone);

#pragma endregion

#pragma region // Scout

	Program.SendUniform("lightAmbient.color", 1.0, 1.0, 1.0);
	Program.SendUniform("materialAmbient", 0.1, 0.1, 0.1);
	Program.SendUniform("lightDir.diffuse", 5.0, 5.0, 5.0);
	Program.SendUniform("materialDiffuse", 0.2f, 0.2f, 0.2f);
	Program.SendUniform("lightSpot[1].on", 1);

	m = matrixView;
	m = translate(m, vec3(0.0f, 0.0f, 0.0f));
	m = rotate(m, radians(-time * 30), vec3(0.0f, 1.0f, 0.0f));
	m = rotate(m, radians(40.0f), vec3(0.0f, 0.0f, 1.0f));
	m = translate(m, vec3(350.0f, -100.0f, 0.0f));
	tempM = m;
	m = scale(m, vec3(2.0f, 2.0f, 2.0f));
	scout.render(m);

	float calc = sinf(time / 1.5f) * 15.0f;

	tempM = rotate(tempM, radians(280.0f + calc), vec3(0.0f, 0.0f, 1.0f));
	tempM = translate(tempM, vec3(1.0f, -10.5f, 1.0f));
	if (isLightOn)
	{
		Program.SendUniform("lightSpot[1].matrix", tempM);
		ProgramTerrain.SendUniform("lightSpot[1].matrix", tempM);
	}

#pragma endregion

#pragma region // Radio

	Program.SendUniform("lightAmbient.color", 1.0, 1.0, 1.0);
	Program.SendUniform("materialAmbient", 0.1, 0.1, 0.1);
	Program.SendUniform("lightDir.diffuse", 2.0, 2.0, 2.0);
	Program.SendUniform("materialDiffuse", 0.2f, 0.2f, 0.2f);

	m = matrixView;
	m = translate(m, vec3(55.0f, 19.7f, -6.5f));
	tempM = m;
	m = rotate(m, radians(180.f), vec3(-0.1f, 1.0f, -0.1f));
	m = scale(m, vec3(0.07f, 0.07f, 0.07f));
	if (isLightOn) radio.render(m);

	if (isLightOn)
	{
		tempM = translate(tempM, vec3(0.0f, 0.5f, -1.2f));
		Program.SendUniform("lightPoint[1].matrix", tempM);
	}


#pragma endregion

#pragma region // Animated Character

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, idCharacter);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, idCharacterN);

	Program.SendUniform("useNormalMap", 1);
	Program.SendUniform("materialAmbient", 0.1, 0.1, 0.1);
	Program.SendUniform("lightDir.diffuse", 5.0, 5.0, 5.0);
	Program.SendUniform("materialDiffuse", 0.2f, 0.2f, 0.2f);

	std::vector<float> transforms;
	character.getAnimData(0, time, transforms);
	Program.SendUniformMatrixv("bones", (float*)&transforms[0], transforms.size() / 16);
	m = matrixView;
	m = translate(m, vec3(54.0f, 16.0f, -8.3f));
	m = rotate(m, radians(180.0f), vec3(0.0f, 1.0f, 0.0f));
	m = scale(m, vec3(3.0f, 3.0f, 3.0f));
	if (animationMode == 0)character.render(m);

	character2.getAnimData(0, time, transforms);
	Program.SendUniformMatrixv("bones", (float*)&transforms[0], transforms.size() / 16);
	m = matrixView;
	m = translate(m, vec3(64.2f, 16.9f, -5.8f));
	m = rotate(m, radians(40.0f), vec3(0.0f, 1.0f, 0.0f));
	m = scale(m, vec3(3.0f, 3.0f, 3.0f));
	if (animationMode == 1) character2.render(m);

	character3.getAnimData(0, time, transforms);
	Program.SendUniformMatrixv("bones", (float*)&transforms[0], transforms.size() / 16);
	m = matrixView;
	m = translate(m, vec3(45.0f, 14.6f, 0.0f));
	m = rotate(m, radians(270.0f), vec3(0.0f, 1.0f, 0.0f));
	m = scale(m, vec3(3.0f, 3.0f, 3.0f));
	if (animationMode == 2) character3.render(m);

	Program.SendUniform("useNormalMap", 0);
	glBindTexture(GL_TEXTURE_2D, idTexNone);

#pragma endregion

#pragma region // Ring
	ProgramTerrain.SendUniform("fogColour2", finalFogColor[0], finalFogColor[1], finalFogColor[2]);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, idTexSandC);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, idTexSandN);

	Program.SendUniform("useNormalMap", 1);

	Program.SendUniform("fogDensity", 0.3);
	Program.SendUniform("materialAmbient", 0.1f, 0.1f, 0.1f);
	Program.SendUniform("materialDiffuse", 0.4f, 0.3f, 0.1f);
	Program.SendUniform("lightDir.diffuse", 1.0, 1.0, 1.0);
	Program.SendUniform("scaleX", 300.0);
	Program.SendUniform("scaleY", 300.0);

	m = matrixView;
	m = translate(m, vec3(0.0f, 20.0f, 0.0f));
	m = rotate(m, radians(0.f), vec3(0.0f, 1.0f, 0.0f));
	m = scale(m, vec3(550.0f, 300.0f, 550.0f));
	ring.render(m);

#pragma endregion

#pragma region // Map 
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, idTexNone);

	ProgramTerrain.SendUniform("useNormalMap", 0);
	ProgramTerrain.SendUniform("useShadowMap", 0);
	ProgramTerrain.SendUniform("fogDensity", 0.3);
	ProgramTerrain.SendUniform("materialAmbient", 0.1f, 0.1f, 0.1f);
	ProgramTerrain.SendUniform("materialDiffuse", finalFogColor[0], finalFogColor[1], finalFogColor[2]);
	ProgramTerrain.SendUniform("lightDir.diffuse", 1.0, 1.0, 1.0);
	ProgramTerrain.SendUniform("scaleX", 1.0);
	ProgramTerrain.SendUniform("scaleY", 1.0);

	ProgramTerrain.Use();

	m = matrixView;
	m = translate(matrixView, vec3(0, -5.0f, 0));
	ProgramTerrain.SendUniform("matrixModelView", m);
	terrain.render(m);

#pragma endregion

#pragma region // Water

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, idTexNone);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, idTexNone);

	ProgramWater.SendUniform("waterColor", finalFogColor[0], finalFogColor[1], finalFogColor[2]);
	finalFogColor = finalFogColor * 3.0f;
	ProgramWater.SendUniform("skyColor", finalFogColor[0], finalFogColor[1], finalFogColor[2]);

	ProgramWater.Use();

	// render the water
	m = matrixView;
	m = translate(m, vec3(0, waterLevel - 5, -75));
	m = rotate(m, radians(45.f), vec3(0.0f, 1.0f, 0.0f));
	m = scale(m, vec3(1.4f, 1.0f, 1.3f));
	ProgramWater.SendUniform("matrixModelView", m);
	water.render(m);

#pragma endregion

#pragma region // Particle System

	glDepthMask(GL_FALSE);				// disable depth buffer updates
	glActiveTexture(GL_TEXTURE0);			// choose the active texture
	glBindTexture(GL_TEXTURE_2D, idTexParticle);	// bind the texture

	// RENDER THE PARTICLE SYSTEM
	ProgramParticle.Use();

	 mat4 viewModel = inverse(matrixView);
	 vec3 camPos(viewModel[3]);

	m = matrixView;
	m = translate(m, vec3(0, 70, 0));
	ProgramParticle.SendUniform("matrixModelView", m);
	ProgramParticle.SendUniform("opacity", transition);

	// render the buffer
	glEnableVertexAttribArray(0);	// velocity
	glEnableVertexAttribArray(1);	// start time
	glEnableVertexAttribArray(2);	// initial position
	glBindBuffer(GL_ARRAY_BUFFER, idBufferVelocity);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, idBufferStartTime);
	glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, idBufferInitialPos);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glDrawArrays(GL_POINTS, 0, NPARTICLES);
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);

	glDepthMask(GL_TRUE);		// don't forget to switch the depth test updates back on

#pragma endregion

}

// Creates a shadow map and stores in idFBO
// theta: current animation control variable
// lightTransform - usually lookAt transform corresponding to the light position and its direction
void createShadowMap(float time, mat4 lightTransform)
{
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);

	// Store the current viewport in a safe place
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	int w = viewport[2];
	int h = viewport[3];

	// setup the viewport to 2x2 the original and wide (120 degrees) FoV (Field of View)
	glViewport(0, 0, w * 2, h * 2);
	mat4 matrixProjection = perspective(radians(120.f), (float)w / (float)h, 0.5f, 50.0f);
	Program.SendUniform("matrixProjection", matrixProjection);
	ProgramTerrain.SendUniform("matrixProjection", matrixProjection);

	// prepare the camera
	mat4 matrixView = lightTransform;

	// send the View Matrix
	Program.SendUniform("matrixView", matrixView);
	ProgramTerrain.SendUniform("matrixView", matrixView);

	// Bind the Framebuffer
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, idFBO2);
	// OFF-SCREEN RENDERING FROM NOW!

	// Clear previous frame values - depth buffer only!
	glClear(GL_DEPTH_BUFFER_BIT);

	// Disable color rendering, we only want to write to the Z-Buffer (this is to speed-up)
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	// Prepare and send the Shadow Matrix - this is matrix transform every coordinate x,y,z
	// x = x* 0.5 + 0.5 
	// y = y* 0.5 + 0.5 
	// z = z* 0.5 + 0.5 
	// Moving from unit cube [-1,1] to [0,1]  
	
	const mat4 bias = {
		{ 0.5, 0.0, 0.0, 0.0 },
		{ 0.0, 0.5, 0.0, 0.0 },
		{ 0.0, 0.0, 0.5, 0.0 },
		{ 0.5, 0.5, 0.5, 1.0 }
	};

	Program.SendUniform("matrixShadow", bias * matrixProjection * matrixView);
	ProgramTerrain.SendUniform("matrixShadow", bias * matrixProjection * matrixView);
	
	// Render all objects in the scene
	renderScene(matrixView, time, false);

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDisable(GL_CULL_FACE);
	void onReshape(int w, int h);
	onReshape(w, h);
}

void prepareCubeMap(float x, float y, float z, float time)
{
	// Store the current viewport in a safe place
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	int w = viewport[2];
	int h = viewport[3];

	// setup the viewport to 256x256, 90 degrees FoV (Field of View)
	glViewport(0, 0, 512, 512);
	Program.SendUniform("matrixProjection", perspective(radians(90.f), 1.0f, 0.02f, 1000.0f));

	// render environment 6 times
	Program.SendUniform("reflectionPower", 0.0);
	for (int i = 0; i < 6; ++i)
	{
		// clear background
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// setup the camera
		const GLfloat ROTATION[6][6] =
		{	// at              up
			{ 1.0, 0.0, 0.0,   0.0, -1.0, 0.0 },  // pos x
			{ -1.0, 0.0, 0.0,  0.0, -1.0, 0.0 },  // neg x
			{ 0.0, 1.0, 0.0,   0.0, 0.0, 1.0 },   // pos y
			{ 0.0, -1.0, 0.0,  0.0, 0.0, -1.0 },  // neg y
			{ 0.0, 0.0, 1.0,   0.0, -1.0, 0.0 },  // poz z
			{ 0.0, 0.0, -1.0,  0.0, -1.0, 0.0 }   // neg z
		};
		mat4 matrixView2 = lookAt(
			vec3(x, y, z),
			vec3(x + ROTATION[i][0], y + ROTATION[i][1], z + ROTATION[i][2]),
			vec3(ROTATION[i][3], ROTATION[i][4], ROTATION[i][5]));

		// send the View Matrix
		Program.SendUniform("matrixView", matrixView2);

		// render scene objects - all but the reflective one
		renderScene(matrixView2, time, false);

		// send the image to the cube texture
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_CUBE_MAP, idTexCube2);
		glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB8, 0, 0, 512, 512, 0);
	}

	// restore the matrixView, viewport and projection
	void onReshape(int w, int h);
	onReshape(w, h);
}

void renderCube(mat4 matrixView, float time)
{
	float calc = sinf(time / 1.5f);


	mat4 m = matrixView;
	Program.SendUniform("scaleX", 1);
	Program.SendUniform("scaleY", 1);
	Program.SendUniform("fogDensity", 0.0);
	Program.SendUniform("lightAmbient.color", 0.5, 0.5, 0.5);
	Program.SendUniform("materialDiffuse", 0.3f, 0.3f, 0.3f);
	Program.SendUniform("materialAmbient", 0.2f, 0.2f, 0.2f);

	Program.SendUniform("useCubeMap", 1);
	Program.SendUniform("reflectionPower", 1.0);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_CUBE_MAP, idTexCube2);

	m = matrixView;
	m = translate(m, vec3(0.0f, 50.0f, 0.0f));
	m = rotate(m, radians(time * 25), vec3(0.0f, 1.0f, 0.0f));
	m = scale(m, vec3(0.2f, 0.2f, 0.2f));
	SFCube.render(m);

	m = matrixView;
	m = translate(m, vec3(0.0f, 50.0f, 0.0f));
	m = scale(m, vec3(18.0f, 18.0f, 18.0f));
	m = rotate(m, radians(20 * 5 * calc), vec3(1.0f, 0.0f, 0.0f));
	ring.render(m);

	m = matrixView;
	m = translate(m, vec3(0.0f, 50.0f, 0.0f));
	m = scale(m, vec3(22.0f, 22.0f, 22.0f));
	m = rotate(m, radians(-25 * 7 * calc), vec3(1.0f, 0.0f, 1.0f));
	ring.render(m);

	m = matrixView;
	m = translate(m, vec3(0.0f, 50.0f, 0.0f));
	m = scale(m, vec3(26.0f, 26.0f, 26.0f));
	m = rotate(m, radians(30 * 9 * calc), vec3(0.0f, 0.0f, 1.0f));
	ring.render(m);

	glActiveTexture(GL_TEXTURE0);
	Program.SendUniform("useCubeMap", 0);
	Program.SendUniform("reflectionPower", 0.0);
}

void prepareCubeMap2(float x, float y, float z, float time)
{
	// Store the current viewport in a safe place
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	int w = viewport[2];
	int h = viewport[3];

	// setup the viewport to 256x256, 90 degrees FoV (Field of View)
	glViewport(0, 0, 512, 512);
	Program.SendUniform("matrixProjection", perspective(radians(90.f), 1.0f, 0.02f, 1000.0f));

	// render environment 6 times
	Program.SendUniform("reflectionPower", 0.0);
	for (int i = 0; i < 6; ++i)
	{
		// clear background
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// setup the camera
		const GLfloat ROTATION[6][6] =
		{	// at              up
			{ 1.0, 0.0, 0.0,   0.0, -1.0, 0.0 },  // pos x
			{ -1.0, 0.0, 0.0,  0.0, -1.0, 0.0 },  // neg x
			{ 0.0, 1.0, 0.0,   0.0, 0.0, 1.0 },   // pos y
			{ 0.0, -1.0, 0.0,  0.0, 0.0, -1.0 },  // neg y
			{ 0.0, 0.0, 1.0,   0.0, -1.0, 0.0 },  // poz z
			{ 0.0, 0.0, -1.0,  0.0, -1.0, 0.0 }   // neg z
		};
		mat4 matrixView2 = lookAt(
			vec3(x, y, z),
			vec3(x + ROTATION[i][0], y + ROTATION[i][1], z + ROTATION[i][2]),
			vec3(ROTATION[i][3], ROTATION[i][4], ROTATION[i][5]));

		// send the View Matrix
		Program.SendUniform("matrixView", matrixView2);

		// render scene objects - all but the reflective one
		renderScene(matrixView2, time, false);
		renderCube(matrixView2, time);

		// send the image to the cube texture
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_CUBE_MAP, idTexCube3);
		glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB8, 0, 0, 512, 512, 0);
	}

	// restore the matrixView, viewport and projection
	void onReshape(int w, int h);
	onReshape(w, h);
}

void renderDeloran(mat4 matrixView, float time)
{
	mat4 m = matrixView;

	Program.SendUniform("fogDensity", 0.05);
	Program.SendUniform("materialAmbient", 0.1, 0.1, 0.1);
	Program.SendUniform("lightDir.diffuse", 8.0, 8.0, 8.0);
	Program.SendUniform("materialDiffuse", 0.2f, 0.2f, 0.2f);

	Program.SendUniform("useCubeMap", 1);
	float reflectionValue = transition - 0.6f;
	if (reflectionValue <= 0) reflectionValue = 0;
	Program.SendUniform("reflectionPower", 0.4f - reflectionValue);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_CUBE_MAP, idTexCube3);

	m = matrixView;
	m = translate(m, vec3(55.0f, 18.0f, -5.0f));
	m = rotate(m, radians(110.0f), vec3(-0.1f, 1.0f, 0.0f));
	m = scale(m, vec3(5.0f, 5.0f, 5.0f));
	delorean.render(m);

	glActiveTexture(GL_TEXTURE0);
	Program.SendUniform("useCubeMap", 0);
	Program.SendUniform("reflectionPower", 0.0);
}

void onRender()
{
	RefreshPostProcessing();

	// this global variable controls the animation
	float time = glutGet(GLUT_ELAPSED_TIME) * 0.001f;

	// send the animation time to shaders
	ProgramWater.SendUniform("t", time);

	ProgramParticle.SendUniform("time", time);

	
	// Shadow map
	createShadowMap(time, lookAt(
		vec3(-2.55f, 50.0f, -1.0f), 	// These are the coordinates of the source of the light
		vec3(0.0f, 3.0f, 0.0f), 		// These are the coordinates of a point behind the scene
		vec3(0.0f, 1.0f, 0.0f)));		// This is just a reasonable "Up" vector);

	

	prepareCubeMap(0.0f, 50.0f, 0.0f, time);
	prepareCubeMap2(55.0f, 18.0f, -5.0f, time);

	// Pass 1: off-screen rendering
	glBindFramebufferEXT(GL_FRAMEBUFFER, idFBO);

	// clear screen and buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	mat4 m = rotate(mat4(1.f), radians(angleTilt), vec3(1.f, 0.f, 0.f));// switch tilt off
	m = translate(m, cam);												// animate camera motion (controlled by WASD keys)
	m = rotate(m, radians(-angleTilt), vec3(1.f, 0.f, 0.f));			// switch tilt on
	matrixView = m * matrixView;

	// move the camera up following the profile of terrain (Y coordinate of the terrain)
	float terrainY = -terrain.getInterpolatedHeight(inverse(matrixView)[3][0], inverse(matrixView)[3][2]);
	matrixView = translate(matrixView, vec3(0, terrainY, 0));;

	Program.SendUniform("matrixView", matrixView);
	ProgramWater.SendUniform("matrixView", matrixView);
	ProgramTerrain.SendUniform("matrixView", matrixView);

	// render the scene objects
	renderScene(matrixView, time, true);

	// render reflected objects
	renderCube(matrixView, time);
	renderDeloran(matrixView, time);

	// the camera must be moved down by terrainY to avoid unwanted effects
	matrixView = translate(matrixView, vec3(0, -terrainY, 0));

	// Pass 2: on-screen rendering
	glBindFramebufferEXT(GL_FRAMEBUFFER, 0);

	// setup ortographic projection
	ProgramEffect.SendUniform("matrixProjection", ortho(0, 1, 0, 1, -1, 1));

	// clear screen and buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBindTexture(GL_TEXTURE_2D, idTexScreen);

	// setup identity matrix as the model-view
	ProgramEffect.SendUniform("matrixModelView", mat4(1));

	GLuint attribVertex = ProgramEffect.GetAttribLocation("aVertex");
	GLuint attribTextCoord = ProgramEffect.GetAttribLocation("aTexCoord");
	glEnableVertexAttribArray(attribVertex);
	glEnableVertexAttribArray(attribTextCoord);
	glBindBuffer(GL_ARRAY_BUFFER, bufQuad);
	glVertexAttribPointer(attribVertex, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0);
	glVertexAttribPointer(attribTextCoord, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glDrawArrays(GL_QUADS, 0, 4);
	glDisableVertexAttribArray(attribVertex);
	glDisableVertexAttribArray(attribTextCoord);

	// essential for double-buffering technique
	glutSwapBuffers();

	// proceed the animation
	glutPostRedisplay();
}

// called before window opened or resized - to setup the Projection Matrix
void onReshape(int w, int h)
{
	float ratio = w * 1.0f / h;      // we hope that h is not zero
	glViewport(0, 0, w, h);
	mat4 matrixProjection = perspective(radians(60.f), ratio, 0.02f, 1000.f);

	// Setup the Projection Matrix
	Program.SendUniform("matrixProjection", matrixProjection);
	ProgramWater.SendUniform("matrixProjection", matrixProjection);
	ProgramTerrain.SendUniform("matrixProjection", matrixProjection);
	ProgramParticle.SendUniform("matrixProjection", matrixProjection);

}

// Handle WASDQE keys
void onKeyDown(unsigned char key, int x, int y)
{
	switch (tolower(key))
	{
	case 'w': cam.z = std::max(cam.z * 1.05f, 1.01f); break;
	case 's': cam.z = std::min(cam.z * 1.05f, -1.01f); break;
	case 'a': cam.x = std::max(cam.x * 1.05f, 1.01f); angleRot = 0.1f; break;
	case 'd': cam.x = std::min(cam.x * 1.05f, -1.01f); angleRot = -0.1f; break;
	case 'e': cam.y = std::max(cam.y * 1.05f, 1.01f); break;
	case 'q': cam.y = std::min(cam.y * 1.05f, -1.01f); break;
	}
	// speed limit
		cam.x = std::max(-0.5f, std::min(0.5f, cam.x));
		cam.y = std::max(-0.5f, std::min(0.5f, cam.y));
		cam.z = std::max(-0.5f, std::min(0.5f, cam.z));
	// stop orbiting
	if ((glutGetModifiers() & GLUT_ACTIVE_SHIFT) == 0) angleRot = 0;
}

// Handle WASDQE keys (key up)
void onKeyUp(unsigned char key, int x, int y)
{
	switch (tolower(key))
	{
	case 'w':
	case 's': cam.z = 0; break;
	case 'a':
	case 'd': cam.x = 0; break;
	case 'q':
	case 'e': cam.y = 0; break;
	case '1': 
		postProcessMode++;
		if (postProcessMode > 6) postProcessMode = 0;
		break;
	case '2':
		animationMode++;
		if (animationMode > 2) animationMode = 0;
		break;
	}
}

// Handle arrow keys and Alt+F4
void onSpecDown(int key, int x, int y)
{
	switch (key)
	{
	case GLUT_KEY_F4:		if ((glutGetModifiers() & GLUT_ACTIVE_ALT) != 0) exit(0); break;
	case GLUT_KEY_UP:		onKeyDown('w', x, y); break;
	case GLUT_KEY_DOWN:		onKeyDown('s', x, y); break;
	case GLUT_KEY_LEFT:		onKeyDown('a', x, y); break;
	case GLUT_KEY_RIGHT:	onKeyDown('d', x, y); break;
	case GLUT_KEY_PAGE_UP:	onKeyDown('q', x, y); break;
	case GLUT_KEY_PAGE_DOWN:onKeyDown('e', x, y); break;
	case GLUT_KEY_F11:		glutFullScreenToggle();
	}
}

// Handle arrow keys (key up)
void onSpecUp(int key, int x, int y)
{
	switch (key)
	{
	case GLUT_KEY_UP:		onKeyUp('w', x, y); break;
	case GLUT_KEY_DOWN:		onKeyUp('s', x, y); break;
	case GLUT_KEY_LEFT:		onKeyUp('a', x, y); break;
	case GLUT_KEY_RIGHT:	onKeyUp('d', x, y); break;
	case GLUT_KEY_PAGE_UP:	onKeyUp('q', x, y); break;
	case GLUT_KEY_PAGE_DOWN:onKeyUp('e', x, y); break;
	}
}

// Handle mouse click
bool bJustClicked = false;
void onMouse(int button, int state, int x, int y)
{
	bJustClicked = (state == GLUT_DOWN);
	glutSetCursor(bJustClicked ? GLUT_CURSOR_CROSSHAIR : GLUT_CURSOR_INHERIT);
	glutWarpPointer(glutGet(GLUT_WINDOW_WIDTH) / 2, glutGet(GLUT_WINDOW_HEIGHT) / 2);
}

// handle mouse move
void onMotion(int x, int y)
{
	if (bJustClicked)
		bJustClicked = false;
	else
	{
		glutWarpPointer(glutGet(GLUT_WINDOW_WIDTH) / 2, glutGet(GLUT_WINDOW_HEIGHT) / 2);

		// find delta (change to) pan & tilt
		float deltaPan = 0.25f * (x - glutGet(GLUT_WINDOW_WIDTH) / 2);
		float deltaTilt = 0.25f * (y - glutGet(GLUT_WINDOW_HEIGHT) / 2);

		// View = Tilt * DeltaPan * Tilt^-1 * DeltaTilt * View;
		angleTilt += deltaTilt;
		mat4 m = mat4(1.f);
		m = rotate(m, radians(angleTilt), vec3(1.f, 0.f, 0.f));
		m = rotate(m, radians(deltaPan), vec3(0.f, 1.f, 0.f));
		m = rotate(m, radians(-angleTilt), vec3(1.f, 0.f, 0.f));
		m = rotate(m, radians(deltaTilt), vec3(1.f, 0.f, 0.f));
		matrixView = m * matrixView;
	}
}

int main(int argc, char **argv)
{
	// init GLUT and create Window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(800, 600);
	glutCreateWindow("CI5520 3D Graphics Programming");

	// init glew
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		cerr << "GLEW Error: " << glewGetErrorString(err) << endl;
		return 0;
	}
	cout << "Using GLEW " << glewGetString(GLEW_VERSION) << endl;

	// register callbacks
	glutDisplayFunc(onRender);
	glutReshapeFunc(onReshape);
	glutKeyboardFunc(onKeyDown);
	glutSpecialFunc(onSpecDown);
	glutKeyboardUpFunc(onKeyUp);
	glutSpecialUpFunc(onSpecUp);
	glutMouseFunc(onMouse);
	glutMotionFunc(onMotion);

	cout << "Vendor: " << glGetString(GL_VENDOR) << endl;
	cout << "Renderer: " << glGetString(GL_RENDERER) << endl;
	cout << "Version: " << glGetString(GL_VERSION) << endl;

	// init light and everything – not a GLUT or callback function!
	if (!init())
	{
		cerr << "Application failed to initialise" << endl;
		return 0;
	}

	// enter GLUT event processing cycle
	glutMainLoop();

	done();

	return 1;
}

