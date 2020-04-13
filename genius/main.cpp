// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <vector>
 
// Include GLEW
#include <GL/glew.h>
 
// Include GLFW
#include <GLFW/glfw3.h>
GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/norm.hpp>
using namespace glm;
 
// Include AntTweakBar
#include <AntTweakBar.h>
#include <common/shader.hpp>
#include <common/texture.hpp>
#include <common/controls.hpp>
#include <common/objloader.hpp>
#include <common/vboindexer.hpp>
#include <common/quaternion_utils.hpp> // See quaternion_utils.cpp for RotationBetweenVectors, LookAt and RotateTowards

vec3 gPosition1(-0.5f, -1.0f, 0.5f);
vec3 gOrientation1;

void loadBuffers(
	std::vector<unsigned short> indices, 
	std::vector<glm::vec3> indexed_vertices, 
	std::vector<glm::vec2> indexed_uvs, 
	std::vector<glm::vec3> indexed_normals,
	GLuint *vertexbuffer,
	GLuint *uvbuffer,
	GLuint *normalbuffer,
	GLuint *elementbuffer
){
	glGenBuffers(1, vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, *vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, indexed_vertices.size() * sizeof(glm::vec3), &indexed_vertices[0], GL_STATIC_DRAW);
 
	glGenBuffers(1, uvbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, *uvbuffer);
	glBufferData(GL_ARRAY_BUFFER, indexed_uvs.size() * sizeof(glm::vec2), &indexed_uvs[0], GL_STATIC_DRAW);
 
	glGenBuffers(1, normalbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, *normalbuffer);
	glBufferData(GL_ARRAY_BUFFER, indexed_normals.size() * sizeof(glm::vec3), &indexed_normals[0], GL_STATIC_DRAW);
 
	glGenBuffers(1, elementbuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *elementbuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned short), &indices[0] , GL_STATIC_DRAW);
}

void bindBuffer(GLuint vertexbuffer, GLuint uvbuffer, GLuint normalbuffer, GLuint elementbuffer, GLuint programID)
{
	GLuint vertexPosition_modelspaceID = glGetAttribLocation(programID, "vertexPosition_modelspace");
	GLuint vertexUVID = glGetAttribLocation(programID, "vertexUV");
	GLuint vertexNormal_modelspaceID = glGetAttribLocation(programID, "vertexNormal_modelspace");
 
	// 1rst attribute buffer: vertices
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glVertexAttribPointer(
		vertexPosition_modelspaceID,  // The attribute we want to configure
		3,                            // size
		GL_FLOAT,                     // type
		GL_FALSE,                     // normalized?
		0,                            // stride
		(void*)0                      // array buffer offset
	);

	// 2nd attribute buffer: UVs
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
	glVertexAttribPointer(
		vertexUVID,                   // The attribute we want to configure
		2,                            // size : U+V => 2
		GL_FLOAT,                     // type
		GL_FALSE,                     // normalized?
		0,                            // stride
		(void*)0                      // array buffer offset
	);

	// 3rd attribute buffer: normals
	glEnableVertexAttribArray(2);
	glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
	glVertexAttribPointer(
		vertexNormal_modelspaceID,    // The attribute we want to configure
		3,                            // size
		GL_FLOAT,                     // type
		GL_FALSE,                     // normalized?
		0,                            // stride
		(void*)0                      // array buffer offset
	);

	// Index buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
}

void deleteBuffers(GLuint vertexbuffer, GLuint uvbuffer, GLuint normalbuffer, GLuint elementbuffer)
{
	glDeleteBuffers(1, &vertexbuffer);
	glDeleteBuffers(1, &uvbuffer);
	glDeleteBuffers(1, &normalbuffer);
	glDeleteBuffers(1, &elementbuffer);
}

void Render(GLuint MatrixID, GLuint Texture, GLuint TextureID, int indiceFinal, glm::mat4 ModelMatrix){
	// Draw the triangles !
	glDrawElements(
		GL_TRIANGLES,        // mode
		indiceFinal,         // count
		GL_UNSIGNED_SHORT,   // type
		(void*)0        // elemenft array buffer offset
	);
}
 
int main( void )
{
	// Initialise GLFW
	if( !glfwInit() )
	{
		fprintf( stderr, "Failed to initialize GLFW\n" );
		getchar();
		return -1;
	}
 
	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
 
	// Open a window and create its OpenGL context
	window = glfwCreateWindow( 1024, 768, "Game Genius - CPII", NULL, NULL);
	if( window == NULL ){
		fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
		getchar();
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		getchar();
		glfwTerminate();
		return -1;
	}
 
	// Initialize the GUI
	TwInit(TW_OPENGL_CORE, NULL);
	TwWindowSize(1024, 768);
	TwBar * EulerGUI = TwNewBar("Euler settings");
	TwSetParam(EulerGUI, NULL, "refresh", TW_PARAM_CSTRING, 1, "0.1");

	TwAddVarRW(EulerGUI, "Euler X", TW_TYPE_FLOAT, &gOrientation1.x, "step=0.01");
	TwAddVarRW(EulerGUI, "Euler Y", TW_TYPE_FLOAT, &gOrientation1.y, "step=0.01");
	TwAddVarRW(EulerGUI, "Euler Z", TW_TYPE_FLOAT, &gOrientation1.z, "step=0.01");
	TwAddVarRW(EulerGUI, "Pos X"  , TW_TYPE_FLOAT, &gPosition1.x, "step=0.1");
	TwAddVarRW(EulerGUI, "Pos Y"  , TW_TYPE_FLOAT, &gPosition1.y, "step=0.1");
	TwAddVarRW(EulerGUI, "Pos Z"  , TW_TYPE_FLOAT, &gPosition1.z, "step=0.1");
 
	// Set GLFW event callbacks. I removed glfwSetWindowSizeCallback for conciseness
	glfwSetMouseButtonCallback(window, (GLFWmousebuttonfun)TwEventMouseButtonGLFW); // - Directly redirect GLFW mouse button events to AntTweakBar
	glfwSetCursorPosCallback(window, (GLFWcursorposfun)TwEventMousePosGLFW);          // - Directly redirect GLFW mouse position events to AntTweakBar
	glfwSetScrollCallback(window, (GLFWscrollfun)TwEventMouseWheelGLFW);    // - Directly redirect GLFW mouse wheel events to AntTweakBar
	glfwSetKeyCallback(window, (GLFWkeyfun)TwEventKeyGLFW);                         // - Directly redirect GLFW key events to AntTweakBar
	glfwSetCharCallback(window, (GLFWcharfun)TwEventCharGLFW);                      // - Directly redirect GLFW char events to AntTweakBar
 
 
	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    // Hide the mouse and enable unlimited mouvement
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_ENABLED);
    
    // Set the mouse at the center of the screen
    glfwPollEvents();
    glfwSetCursorPos(window, 1024/2, 768/2);
 
	// Dark blue background
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
 
	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS); 
 
	// Cull triangles which normal is not towards the camera
	glEnable(GL_CULL_FACE);
 
	// Create and compile our GLSL program from the shaders
	GLuint programID = LoadShaders( "StandardShading.vertexshader", "StandardShading.fragmentshader" );
 
	// Get a handle for our "MVP" uniform
	GLuint MatrixID = glGetUniformLocation(programID, "MVP");
	GLuint ViewMatrixID = glGetUniformLocation(programID, "V");
	GLuint ModelMatrixID = glGetUniformLocation(programID, "M");

	// Load the texture
	GLuint Texture_mesa_toda = loadDDS("mesa_jogo.dds");
	GLuint Texture_botao_amarelo = loadDDS("uv-botao-AMARELO-Pronta.dds");
	GLuint Texture_botao_azul = loadDDS("uv-botao-azul-COMPLETO.dds");
	GLuint Texture_botao_verde = loadDDS("uv-botao-verde-pronta.dds");
	GLuint Texture_botao_vermelho = loadDDS("uv-botao-vermelho-pronto.dds");
	GLuint Texture_botaozinho_direito = loadDDS("uv-botazinho-centro-direto-amarelo-PRONTo.dds");
	GLuint Texture_botaozinho_centro = loadDDS("uv-botazinho-centro-esquerdo-amarelo-PRONTO.dds");
	GLuint Texture_botaozinho_esquerdo = loadDDS("uv-botazinho-centro--vermlho-PRONTO.dds");
	GLuint Texture_circulo_central = loadDDS("uv-centro-circulo-genius-pronto.dds");
	GLuint Texture_genius_externo = loadDDS("uv-Jogo-Inteiro-Parte-Preta-pronto.dds");
	
	// Get a handle for our "myTextureSampler" uniform
	GLuint TextureID  = glGetUniformLocation(programID, "myTextureSampler");
 
	// Read our .obj file
	/*
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals;
	bool mesa = loadOBJ("mesa.obj", vertices, uvs, normals);
	bool botaoAmarelo = loadOBJ("botaoAmarelo.obj", vertices, uvs, normals);
	bool botaoAzul = loadOBJ("botaoAzul.obj", vertices, uvs, normals);
	bool botaoVerde = loadOBJ("botaoVerde.obj", vertices, uvs, normals);
	bool botaoVermelho = loadOBJ("botaoVermelho.obj", vertices, uvs, normals);
	bool botaoAmareloEsquerdo = loadOBJ("botaoAmareloEsquerdo.obj", vertices, uvs, normals);
	bool botaoAmareloDireito = loadOBJ("botaoAmareloDireito.obj", vertices, uvs, normals);
	bool botaoVermelhoMeio = loadOBJ("botaoVermelhoMeio.obj", vertices, uvs, normals);
	bool restoJogo = loadOBJ("restoJogo.obj", vertices, uvs, normals);
	*/

	// BOTAO AMARELO
	std::vector<glm::vec3> botaoAmareloVertices;
	std::vector<glm::vec2> botaoAmareloUvs;
	std::vector<glm::vec3> botaoAmareloNormals;
	std::vector<unsigned short> botaoAmareloIndices;
	std::vector<glm::vec3> botaoAmareloIndexedVertices;
	std::vector<glm::vec2> botaoAmareloIndexedUvs;
	std::vector<glm::vec3> botaoAmareloIndexedNormals;
	GLuint botaoAmareloVertexbuffer;
	GLuint botaoAmareloUvbuffer;
	GLuint botaoAmareloNormalbuffer;
	GLuint botaoAmareloElementbuffer;
	bool botaoAmarelo = loadOBJ("botaoAmarelo.obj", botaoAmareloVertices, botaoAmareloUvs, botaoAmareloNormals);
	// chama load buffers e cria buffer para o botao tal;
	indexVBO(botaoAmareloVertices, botaoAmareloUvs, botaoAmareloNormals, botaoAmareloIndices, botaoAmareloIndexedVertices, botaoAmareloIndexedUvs, botaoAmareloIndexedNormals);
	loadBuffers(
		botaoAmareloIndices, 
		botaoAmareloIndexedVertices, 
		botaoAmareloIndexedUvs, 
		botaoAmareloIndexedNormals, 
		&botaoAmareloVertexbuffer, 
		&botaoAmareloUvbuffer, 
		&botaoAmareloNormalbuffer, 
		&botaoAmareloElementbuffer
	);

	// BOTAO AZUL
	std::vector<glm::vec3> botaoAzulVertices;
	std::vector<glm::vec2> botaoAzulUvs;
	std::vector<glm::vec3> botaoAzulNormals;
	std::vector<unsigned short> botaoAzulIndices;
	std::vector<glm::vec3> botaoAzulIndexedVertices;
	std::vector<glm::vec2> botaoAzulIndexedUvs;
	std::vector<glm::vec3> botaoAzulIndexedNormals;
	GLuint botaoAzulVertexbuffer;
	GLuint botaoAzulUvbuffer;
	GLuint botaoAzulNormalbuffer;
	GLuint botaoAzulElementbuffer;

	bool botaoAzul = loadOBJ("botaoAzul.obj", botaoAzulVertices, botaoAzulUvs, botaoAzulNormals);
	indexVBO(botaoAzulVertices, botaoAzulUvs, botaoAzulNormals, botaoAzulIndices, botaoAzulIndexedVertices, botaoAzulIndexedUvs, botaoAzulIndexedNormals);
	loadBuffers( 
		botaoAzulIndices, 
		botaoAzulIndexedVertices, 
		botaoAzulIndexedUvs, 
		botaoAzulIndexedNormals, 
		&botaoAzulVertexbuffer, 
		&botaoAzulUvbuffer, 
		&botaoAzulNormalbuffer, 
		&botaoAzulElementbuffer
	);

	// BOTAO VERDE
	std::vector<glm::vec3> botaoVerdeVertices;
	std::vector<glm::vec2> botaoVerdeUvs;
	std::vector<glm::vec3> botaoVerdeNormals;
	std::vector<unsigned short> botaoVerdeIndices;
	std::vector<glm::vec3> botaoVerdeIndexedVertices;
	std::vector<glm::vec2> botaoVerdeIndexedUvs;
	std::vector<glm::vec3> botaoVerdeIndexedNormals;
	GLuint botaoVerdeVertexbuffer;
	GLuint botaoVerdeUvbuffer;
	GLuint botaoVerdeNormalbuffer;
	GLuint botaoVerdeElementbuffer;

	bool botaoVerde = loadOBJ("botaoVerde.obj", botaoVerdeVertices, botaoVerdeUvs, botaoVerdeNormals);
	indexVBO(botaoVerdeVertices, botaoVerdeUvs, botaoVerdeNormals, botaoVerdeIndices, botaoVerdeIndexedVertices, botaoVerdeIndexedUvs, botaoVerdeIndexedNormals);
	loadBuffers( 
		botaoVerdeIndices, 
		botaoVerdeIndexedVertices, 
		botaoVerdeIndexedUvs, 
		botaoVerdeIndexedNormals, 
		&botaoVerdeVertexbuffer, 
		&botaoVerdeUvbuffer, 
		&botaoVerdeNormalbuffer, 
		&botaoVerdeElementbuffer
	);

	// BOTAO VERMELHO
	std::vector<glm::vec3> botaoVermelhoVertices;
	std::vector<glm::vec2> botaoVermelhoUvs;
	std::vector<glm::vec3> botaoVermelhoNormals;
	std::vector<unsigned short> botaoVermelhoIndices;
	std::vector<glm::vec3> botaoVermelhoIndexedVertices;
	std::vector<glm::vec2> botaoVermelhoIndexedUvs;
	std::vector<glm::vec3> botaoVermelhoIndexedNormals;
	GLuint botaoVermelhoVertexbuffer;
	GLuint botaoVermelhoUvbuffer;
	GLuint botaoVermelhoNormalbuffer;
	GLuint botaoVermelhoElementbuffer;

	bool botaoVermelho = loadOBJ("botaoVermelho.obj", botaoVermelhoVertices, botaoVermelhoUvs, botaoVermelhoNormals);
	indexVBO(botaoVermelhoVertices, botaoVermelhoUvs, botaoVermelhoNormals, botaoVermelhoIndices, botaoVermelhoIndexedVertices, botaoVermelhoIndexedUvs, botaoVermelhoIndexedNormals);
	loadBuffers(
		botaoVermelhoIndices, 
		botaoVermelhoIndexedVertices, 
		botaoVermelhoIndexedUvs, 
		botaoVermelhoIndexedNormals, 
		&botaoVermelhoVertexbuffer, 
		&botaoVermelhoUvbuffer, 
		&botaoVermelhoNormalbuffer, 
		&botaoVermelhoElementbuffer
	);

	// BOTAO AMARELO ESQUERDO
	std::vector<glm::vec3> botaoAmareloEsquerdoVertices;
	std::vector<glm::vec2> botaoAmareloEsquerdoUvs;
	std::vector<glm::vec3> botaoAmareloEsquerdoNormals;
	std::vector<unsigned short> botaoAmareloEsquerdoIndices;
	std::vector<glm::vec3> botaoAmareloEsquerdoIndexedVertices;
	std::vector<glm::vec2> botaoAmareloEsquerdoIndexedUvs;
	std::vector<glm::vec3> botaoAmareloEsquerdoIndexedNormals;
	GLuint botaoAmareloEsquerdoVertexbuffer;
	GLuint botaoAmareloEsquerdoUvbuffer;
	GLuint botaoAmareloEsquerdoNormalbuffer;
	GLuint botaoAmareloEsquerdoElementbuffer;

	bool botaoAmareloEsquerdo = loadOBJ("botaoAmareloEsquerdo.obj", botaoAmareloEsquerdoVertices, botaoAmareloEsquerdoUvs, botaoAmareloEsquerdoNormals);
	indexVBO(botaoAmareloEsquerdoVertices, botaoAmareloEsquerdoUvs, botaoAmareloEsquerdoNormals, botaoAmareloEsquerdoIndices, botaoAmareloEsquerdoIndexedVertices, botaoAmareloEsquerdoIndexedUvs, botaoAmareloEsquerdoIndexedNormals);
	loadBuffers( 
		botaoAmareloEsquerdoIndices, 
		botaoAmareloEsquerdoIndexedVertices, 
		botaoAmareloEsquerdoIndexedUvs, 
		botaoAmareloEsquerdoIndexedNormals, 
		&botaoAmareloEsquerdoVertexbuffer, 
		&botaoAmareloEsquerdoUvbuffer, 
		&botaoAmareloEsquerdoNormalbuffer, 
		&botaoAmareloEsquerdoElementbuffer
	);

	// BOTAO AMARELO DIREITO 
	std::vector<glm::vec3> botaoAmareloDireitoVertices;
	std::vector<glm::vec2> botaoAmareloDireitoUvs;
	std::vector<glm::vec3> botaoAmareloDireitoNormals;
	std::vector<unsigned short> botaoAmareloDireitoIndices;
	std::vector<glm::vec3> botaoAmareloDireitoIndexedVertices;
	std::vector<glm::vec2> botaoAmareloDireitoIndexedUvs;
	std::vector<glm::vec3> botaoAmareloDireitoIndexedNormals;
	GLuint botaoAmareloDireitoVertexbuffer;
	GLuint botaoAmareloDireitoUvbuffer;
	GLuint botaoAmareloDireitoNormalbuffer;
	GLuint botaoAmareloDireitoElementbuffer;

	bool botaoAmareloDireito = loadOBJ("botaoAmareloDireito.obj", botaoAmareloDireitoVertices, botaoAmareloDireitoUvs, botaoAmareloDireitoNormals);
	indexVBO(botaoAmareloDireitoVertices, botaoAmareloDireitoUvs, botaoAmareloDireitoNormals, botaoAmareloDireitoIndices, botaoAmareloDireitoIndexedVertices, botaoAmareloDireitoIndexedUvs, botaoAmareloDireitoIndexedNormals);
	loadBuffers(
		botaoAmareloDireitoIndices, 
		botaoAmareloDireitoIndexedVertices, 
		botaoAmareloDireitoIndexedUvs, 
		botaoAmareloDireitoIndexedNormals, 
		&botaoAmareloDireitoVertexbuffer, 
		&botaoAmareloDireitoUvbuffer, 
		&botaoAmareloDireitoNormalbuffer, 
		&botaoAmareloDireitoElementbuffer
	);

	// BOTAO VERMELHO MEIO
	std::vector<glm::vec3> botaoVermelhoMeioVertices;
	std::vector<glm::vec2> botaoVermelhoMeioUvs;
	std::vector<glm::vec3> botaoVermelhoMeioNormals;
	std::vector<unsigned short> botaoVermelhoMeioIndices;
	std::vector<glm::vec3> botaoVermelhoMeioIndexedVertices;
	std::vector<glm::vec2> botaoVermelhoMeioIndexedUvs;
	std::vector<glm::vec3> botaoVermelhoMeioIndexedNormals;
	GLuint botaoVermelhoMeioVertexbuffer;
	GLuint botaoVermelhoMeioUvbuffer;
	GLuint botaoVermelhoMeioNormalbuffer;
	GLuint botaoVermelhoMeioElementbuffer;

	bool botaoVermelhoMeio = loadOBJ("botaoVermelhoMeio.obj", botaoVermelhoMeioVertices, botaoVermelhoMeioUvs, botaoVermelhoMeioNormals);
	indexVBO(botaoVermelhoMeioVertices, botaoVermelhoMeioUvs, botaoVermelhoMeioNormals, botaoVermelhoMeioIndices, botaoVermelhoMeioIndexedVertices, botaoVermelhoMeioIndexedUvs, botaoVermelhoMeioIndexedNormals);
	loadBuffers(
		botaoVermelhoMeioIndices, 
		botaoVermelhoMeioIndexedVertices, 
		botaoVermelhoMeioIndexedUvs, 
		botaoVermelhoMeioIndexedNormals, 
		&botaoVermelhoMeioVertexbuffer, 
		&botaoVermelhoMeioUvbuffer, 
		&botaoVermelhoMeioNormalbuffer, 
		&botaoVermelhoMeioElementbuffer
	);

	std::vector<glm::vec3> mesaVertices;
	std::vector<glm::vec2> mesaUvs;
	std::vector<glm::vec3> mesaNormals;

	std::vector<unsigned short> mesaIndices;
	std::vector<glm::vec3> mesaIndexedVertices;
	std::vector<glm::vec2> mesaIndexedUvs;
	std::vector<glm::vec3> mesaIndexedNormals;

	GLuint mesaVertexbuffer;
	GLuint mesaUvbuffer;
	GLuint mesaNormalbuffer;
	GLuint mesaElementbuffer;

	bool mesa = loadOBJ("mesa.obj", mesaVertices, mesaUvs, mesaNormals);
	indexVBO(mesaVertices, mesaUvs, mesaNormals, mesaIndices, mesaIndexedVertices, mesaIndexedUvs, mesaIndexedNormals);
	loadBuffers(
		mesaIndices, 
		mesaIndexedVertices, 
		mesaIndexedUvs, 
		mesaIndexedNormals, 
		&mesaVertexbuffer, 
		&mesaUvbuffer, 
		&mesaNormalbuffer, 
		&mesaElementbuffer
	);

	std::vector<glm::vec3> restoJogoVertices;
	std::vector<glm::vec2> restoJogoUvs;
	std::vector<glm::vec3> restoJogoNormals;

	std::vector<unsigned short> restoJogoIndices;
	std::vector<glm::vec3> restoJogoIndexedVertices;
	std::vector<glm::vec2> restoJogoIndexedUvs;
	std::vector<glm::vec3> restoJogoIndexedNormals;

	GLuint restoJogoVertexbuffer;
	GLuint restoJogoUvbuffer;
	GLuint restoJogoNormalbuffer;
	GLuint restoJogoElementbuffer;

	bool restoJogo = loadOBJ("restoJogo.obj", restoJogoVertices, restoJogoUvs, restoJogoNormals);
	indexVBO(restoJogoVertices, restoJogoUvs, restoJogoNormals, restoJogoIndices, restoJogoIndexedVertices, restoJogoIndexedUvs, restoJogoIndexedNormals);
	loadBuffers(
		restoJogoIndices, 
		restoJogoIndexedVertices, 
		restoJogoIndexedUvs, 
		restoJogoIndexedNormals, 
		&restoJogoVertexbuffer, 
		&restoJogoUvbuffer, 
		&restoJogoNormalbuffer, 
		&restoJogoElementbuffer
	);

	std::vector<glm::vec3> meioRestoJogoVertices;
	std::vector<glm::vec2> meioRestoJogoUvs;
	std::vector<glm::vec3> meioRestoJogoNormals;

	std::vector<unsigned short> meioRestoJogoIndices;
	std::vector<glm::vec3> meioRestoJogoIndexedVertices;
	std::vector<glm::vec2> meioRestoJogoIndexedUvs;
	std::vector<glm::vec3> meioRestoJogoIndexedNormals;

	GLuint meioRestoJogoVertexbuffer;
	GLuint meioRestoJogoUvbuffer;
	GLuint meioRestoJogoNormalbuffer;
	GLuint meioRestoJogoElementbuffer;

	bool meioRestoJogo = loadOBJ("meioRestoJogo.obj", meioRestoJogoVertices, meioRestoJogoUvs, meioRestoJogoNormals);
	indexVBO(meioRestoJogoVertices, meioRestoJogoUvs, meioRestoJogoNormals, meioRestoJogoIndices, meioRestoJogoIndexedVertices, meioRestoJogoIndexedUvs, meioRestoJogoIndexedNormals);
	loadBuffers(
		meioRestoJogoIndices, 
		meioRestoJogoIndexedVertices, 
		meioRestoJogoIndexedUvs, 
		meioRestoJogoIndexedNormals, 
		&meioRestoJogoVertexbuffer, 
		&meioRestoJogoUvbuffer, 
		&meioRestoJogoNormalbuffer, 
		&meioRestoJogoElementbuffer
	);

	// Get a handle for our "LightPosition" uniform
	glUseProgram(programID);
	GLuint LightID = glGetUniformLocation(programID, "LightPosition_worldspace");
 
	// For speed computation
	double lastTime = glfwGetTime();
	double lastFrameTime = lastTime;
	int nbFrames = 0;

	glm::vec3 cameraFrontPosition = glm::vec3(0, 5, 15);
	glm::vec3 cameraBackPosition = glm::vec3(0, 5, -15);
	glm::vec3 cameraTopPosition = glm::vec3(0, 10, 0);

	glm::vec3 cameraNormalLookTo = glm::vec3(0, 1, 0);
	glm::vec3 cameraTopLookTo = glm::vec3(0, 1, -1);

	glm::vec3 cameraHeadNormal = glm::vec3(0, 1, 0);
	glm::vec3 cameraHeadUpsideDown = glm::vec3(0, -1, 0);

	glm::vec3 cameraPosition = cameraFrontPosition;
	glm::vec3 cameraLookTo = cameraNormalLookTo;
	glm::vec3 cameraHead = cameraHeadNormal;

	bool animacao = false;
	bool zSomar = false;
	bool visualizarOrtho = false;
	double pKeyTimePressed;

	glm::mat4 perspectiveProjection = glm::perspective(glm::radians(45.0f), 4.0f / 3.0f, 0.1f, 100.0f);
	glm::mat4 ortogonalProjection = glm::ortho(-10.0f,10.0f,-10.0f,10.0f,0.0f,100.0f);
	glm::mat4 ProjectionMatrix = perspectiveProjection;

	do {

		// Measure speed
		double currentTime = glfwGetTime();
		float deltaTime = (float)(currentTime - lastFrameTime); 
		lastFrameTime = currentTime;
		nbFrames++;
		if ( currentTime - lastTime >= 1.0 ){ // If last prinf() was more than 1sec ago
			// printf and reset
			printf("%f ms/frame\n", 1000.0/double(nbFrames));
			nbFrames = 0;
			lastTime += 1.0;
		}
 
		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
 
		// Use our shader
		glUseProgram(programID);
 
		if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
			if (!pKeyTimePressed || (currentTime - pKeyTimePressed) > 0.4) {
				visualizarOrtho = !visualizarOrtho;
				pKeyTimePressed = glfwGetTime();
			}
		}

		if (visualizarOrtho) {
			ProjectionMatrix = ortogonalProjection;
		} else {
			ProjectionMatrix = perspectiveProjection;
		}

		if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS) {
			animacao = false;
			cameraPosition = cameraFrontPosition;
			cameraLookTo = cameraNormalLookTo;
			cameraHead = cameraHeadNormal;
			gPosition1.z = 0.5f;
		}

		if (glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS) {
			animacao = false;
			cameraPosition = cameraTopPosition;
			cameraLookTo = cameraTopLookTo;
			cameraHead = cameraHeadNormal;
			gPosition1.z = 0.0f;
		}

		if (glfwGetKey(window, GLFW_KEY_F3) == GLFW_PRESS) {
			animacao = false;
			cameraPosition = cameraBackPosition;
			cameraLookTo = cameraNormalLookTo;
			cameraHead = cameraHeadNormal;
			gPosition1.z = 0.5f;
		}

		if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {
			animacao = true;
			zSomar = false;
			cameraPosition = cameraFrontPosition;
			cameraLookTo = cameraNormalLookTo;
			cameraHead = cameraHeadNormal;
		}

		if (animacao) {
			if (cameraPosition.y < 10 && !zSomar) {
				cameraPosition.y += 5.0f * deltaTime;
			}

			if (cameraPosition.y > 10) {

				if (zSomar) {
					cameraPosition.z += 5.0f * deltaTime;
				} else {
					cameraPosition.z -= 5.0f * deltaTime;
				}

				if (cameraPosition.z < 0) {
					zSomar = true;
					cameraPosition.z *= -1;
					cameraLookTo.z *= -1;
				}
			}

			if (cameraPosition.z > 15 && zSomar) {
				if (cameraPosition.y > 5) {
					cameraPosition.y -= 5.0f * deltaTime;
				} else {
					animacao = false;
					zSomar = false;
					cameraPosition = cameraFrontPosition;
					cameraLookTo = cameraNormalLookTo;
					cameraHead = cameraHeadNormal;
				}
			}
		}

		glm::mat4 ViewMatrix = glm::lookAt(
			cameraPosition, // Camera is here
			cameraLookTo, // and looks here
			cameraHead  // Head is up (set to 0,-1,0 to look upside-down)
		);
 
		glm::vec3 lightPos = glm::vec3(0, 8, -1);
		glUniform3f(LightID, lightPos.x, lightPos.y, lightPos.z);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture_mesa_toda);
		glUniform1i(TextureID, 0);

 		bindBuffer(mesaVertexbuffer, mesaUvbuffer, mesaNormalbuffer, mesaElementbuffer, programID);
		{
			glm::mat4 RotationMatrix = eulerAngleYXZ(gOrientation1.y, gOrientation1.x, gOrientation1.z);
			glm::mat4 TranslationMatrix = translate(mat4(), gPosition1); // A bit to the left
			glm::mat4 ScalingMatrix = scale(mat4(), vec3(1.0f, 1.0f, 1.0f));
			glm::mat4 ModelMatrix = TranslationMatrix * RotationMatrix * ScalingMatrix;
			glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

			glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
			glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
			glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);
			
			glDrawElements(GL_TRIANGLES, mesaIndices.size(), GL_UNSIGNED_SHORT, (void*) 0);
			// Render(MatrixID, Texture_mesa_toda, TextureID, mesaIndices.size(), ModelMatrix);
		}

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, Texture_botao_amarelo);
		glUniform1i(TextureID, 0);
		bindBuffer(botaoAmareloVertexbuffer, botaoAmareloUvbuffer, botaoAmareloNormalbuffer, botaoAmareloElementbuffer, programID);
 		{
			glm::mat4 RotationMatrix = eulerAngleYXZ(gOrientation1.y, gOrientation1.x, gOrientation1.z);
			glm::mat4 TranslationMatrix = translate(mat4(), gPosition1); // A bit to the left
			glm::mat4 ScalingMatrix = scale(mat4(), vec3(1.0f, 1.0f, 1.0f));
			glm::mat4 ModelMatrix = TranslationMatrix * RotationMatrix * ScalingMatrix;
			glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

			glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
			glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
			glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);

			glDrawElements(GL_TRIANGLES, botaoAmareloIndices.size(), GL_UNSIGNED_SHORT, (void*) 0);

			// Render(MatrixID, Texture_botao_amarelo, TextureID, botaoAmareloIndices.size(), ModelMatrix);
		}

		// glActiveTexture(GL_TEXTURE2);
		// glBindTexture(GL_TEXTURE_2D, Texture_botao_azul);
		// glUniform1i(TextureID, 0);
		// bindBuffer(botaoAzulVertexbuffer, botaoAzulUvbuffer, botaoAzulNormalbuffer, botaoAzulElementbuffer, programID);
		// {
		// 	glm::mat4 RotationMatrix = eulerAngleYXZ(gOrientation1.y, gOrientation1.x, gOrientation1.z);
		// 	glm::mat4 TranslationMatrix = translate(mat4(), gPosition1); // A bit to the left
		// 	glm::mat4 ScalingMatrix = scale(mat4(), vec3(1.0f, 1.0f, 1.0f));
		// 	glm::mat4 ModelMatrix = TranslationMatrix * RotationMatrix * ScalingMatrix;
		// 	glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
			
		// 	glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
		// 	glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
		// 	glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);
		// 	Render(MatrixID, Texture_botao_azul, TextureID, botaoAzulIndices.size(), ModelMatrix);
		// }

		// glActiveTexture(GL_TEXTURE3);
		// glBindTexture(GL_TEXTURE_2D, Texture_botao_verde);
		// glUniform1i(TextureID, 0);
 	// 	bindBuffer(botaoVerdeVertexbuffer, botaoVerdeUvbuffer, botaoVerdeNormalbuffer, botaoVerdeElementbuffer, programID);
 	// 	{
		// 	glm::mat4 RotationMatrix = eulerAngleYXZ(gOrientation1.y, gOrientation1.x, gOrientation1.z);
		// 	glm::mat4 TranslationMatrix = translate(mat4(), gPosition1); // A bit to the left
		// 	glm::mat4 ScalingMatrix = scale(mat4(), vec3(1.0f, 1.0f, 1.0f));
		// 	glm::mat4 ModelMatrix = TranslationMatrix * RotationMatrix * ScalingMatrix;
		// 	glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
			
		// 	glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
		// 	glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
		// 	glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);
		// 	Render(MatrixID, Texture_botao_verde, TextureID, botaoVerdeIndices.size(), ModelMatrix);
		// }

		// glActiveTexture(GL_TEXTURE4);
		// glBindTexture(GL_TEXTURE_2D, Texture_botao_vermelho);
		// glUniform1i(TextureID, 0);
 	// 	bindBuffer(botaoVermelhoVertexbuffer, botaoVermelhoUvbuffer, botaoVermelhoNormalbuffer, botaoVermelhoElementbuffer, programID);
 	// 	{
		// 	glm::mat4 RotationMatrix = eulerAngleYXZ(gOrientation1.y, gOrientation1.x, gOrientation1.z);
		// 	glm::mat4 TranslationMatrix = translate(mat4(), gPosition1); // A bit to the left
		// 	glm::mat4 ScalingMatrix = scale(mat4(), vec3(1.0f, 1.0f, 1.0f));
		// 	glm::mat4 ModelMatrix = TranslationMatrix * RotationMatrix * ScalingMatrix;
		// 	glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
			
		// 	glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
		// 	glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
		// 	glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);
		// 	Render(MatrixID, Texture_botao_vermelho, TextureID, botaoVermelhoIndices.size(), ModelMatrix);
		// }

		// glActiveTexture(GL_TEXTURE7);
		// glBindTexture(GL_TEXTURE_2D, Texture_botaozinho_esquerdo);
		// glUniform1i(TextureID, 0);
 	// 	bindBuffer(botaoAmareloEsquerdoVertexbuffer, botaoAmareloEsquerdoUvbuffer, botaoAmareloEsquerdoNormalbuffer, botaoAmareloEsquerdoElementbuffer, programID);
 	// 	{
		// 	glm::mat4 RotationMatrix = eulerAngleYXZ(gOrientation1.y, gOrientation1.x, gOrientation1.z);
		// 	glm::mat4 TranslationMatrix = translate(mat4(), gPosition1); // A bit to the left
		// 	glm::mat4 ScalingMatrix = scale(mat4(), vec3(1.0f, 1.0f, 1.0f));
		// 	glm::mat4 ModelMatrix = TranslationMatrix * RotationMatrix * ScalingMatrix;
		// 	glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
			
		// 	glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
		// 	glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
		// 	glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);
		// 	Render(MatrixID, Texture_botaozinho_esquerdo, TextureID, botaoAmareloEsquerdoIndices.size(), ModelMatrix);
		// }

		// glActiveTexture(GL_TEXTURE5);
		// glBindTexture(GL_TEXTURE_2D, Texture_botaozinho_direito);
		// glUniform1i(TextureID, 0);
 	// 	bindBuffer(botaoAmareloDireitoVertexbuffer, botaoAmareloDireitoUvbuffer, botaoAmareloDireitoNormalbuffer, botaoAmareloDireitoElementbuffer, programID);
 	// 	{
		// 	glm::mat4 RotationMatrix = eulerAngleYXZ(gOrientation1.y, gOrientation1.x, gOrientation1.z);
		// 	glm::mat4 TranslationMatrix = translate(mat4(), gPosition1); // A bit to the left
		// 	glm::mat4 ScalingMatrix = scale(mat4(), vec3(1.0f, 1.0f, 1.0f));
		// 	glm::mat4 ModelMatrix = TranslationMatrix * RotationMatrix * ScalingMatrix;
		// 	glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
			
		// 	glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
		// 	glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
		// 	glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);
		// 	Render(MatrixID, Texture_botaozinho_direito, TextureID, botaoAmareloDireitoIndices.size(), ModelMatrix);
		// }

		// glActiveTexture(GL_TEXTURE6);
		// glBindTexture(GL_TEXTURE_2D, Texture_botaozinho_centro);
		// glUniform1i(TextureID, 0);
 	// 	bindBuffer(botaoVermelhoMeioVertexbuffer, botaoVermelhoMeioUvbuffer, botaoVermelhoMeioNormalbuffer, botaoVermelhoMeioElementbuffer, programID);
 	// 	{
		// 	glm::mat4 RotationMatrix = eulerAngleYXZ(gOrientation1.y, gOrientation1.x, gOrientation1.z);
		// 	glm::mat4 TranslationMatrix = translate(mat4(), gPosition1); // A bit to the left
		// 	glm::mat4 ScalingMatrix = scale(mat4(), vec3(1.0f, 1.0f, 1.0f));
		// 	glm::mat4 ModelMatrix = TranslationMatrix * RotationMatrix * ScalingMatrix;
		// 	glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
			
		// 	glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
		// 	glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
		// 	glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);
		// 	Render(MatrixID, Texture_botaozinho_centro, TextureID, botaoVermelhoMeioIndices.size(), ModelMatrix);
		// }

		// glActiveTexture(GL_TEXTURE9);
		// glBindTexture(GL_TEXTURE_2D, Texture_genius_externo);
		// glUniform1i(TextureID, 0);
		// bindBuffer(restoJogoVertexbuffer, restoJogoUvbuffer, restoJogoNormalbuffer, restoJogoElementbuffer, programID);
		// {
		// 	glm::mat4 RotationMatrix = eulerAngleYXZ(gOrientation1.y, gOrientation1.x, gOrientation1.z);
		// 	glm::mat4 TranslationMatrix = translate(mat4(), gPosition1); // A bit to the left
		// 	glm::mat4 ScalingMatrix = scale(mat4(), vec3(1.0f, 1.0f, 1.0f));
		// 	glm::mat4 ModelMatrix = TranslationMatrix * RotationMatrix * ScalingMatrix;
		// 	glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
			
		// 	glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
		// 	glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
		// 	glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);
		// 	Render(MatrixID, Texture_genius_externo, TextureID, restoJogoIndices.size(), ModelMatrix);
		// }

		// glActiveTexture(GL_TEXTURE8);
		// glBindTexture(GL_TEXTURE_2D, Texture_circulo_central);
		// glUniform1i(TextureID, 0);
		// bindBuffer(meioRestoJogoVertexbuffer, meioRestoJogoUvbuffer, meioRestoJogoNormalbuffer, meioRestoJogoElementbuffer, programID);
		// {
		// 	glm::mat4 RotationMatrix = eulerAngleYXZ(gOrientation1.y, gOrientation1.x, gOrientation1.z);
		// 	glm::mat4 TranslationMatrix = translate(mat4(), gPosition1); // A bit to the left
		// 	glm::mat4 ScalingMatrix = scale(mat4(), vec3(1.0f, 1.0f, 1.0f));
		// 	glm::mat4 ModelMatrix = TranslationMatrix * RotationMatrix * ScalingMatrix;
		// 	glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
			
		// 	glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
		// 	glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
		// 	glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);
		// 	Render(MatrixID, Texture_circulo_central, TextureID, meioRestoJogoIndices.size(), ModelMatrix);
		// }

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);
 
		// Draw GUI
		TwDraw();
 
		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();
 
	} // Check if the ESC key was pressed or the window was closed

	while( 
		glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS && glfwWindowShouldClose(window) == 0 
	);
 
	// Cleanup VBO and shader
	deleteBuffers(botaoAmareloVertexbuffer, botaoAmareloUvbuffer, botaoAmareloNormalbuffer, botaoAmareloElementbuffer);
	deleteBuffers(botaoAzulVertexbuffer, botaoAzulUvbuffer, botaoAzulNormalbuffer, botaoAzulElementbuffer);
	deleteBuffers(botaoVerdeVertexbuffer, botaoVerdeUvbuffer, botaoVerdeNormalbuffer, botaoVerdeElementbuffer);
	deleteBuffers(botaoVermelhoVertexbuffer, botaoVermelhoUvbuffer, botaoVermelhoNormalbuffer, botaoVermelhoElementbuffer);
	deleteBuffers(botaoAmareloEsquerdoVertexbuffer, botaoAmareloEsquerdoUvbuffer, botaoAmareloEsquerdoNormalbuffer, botaoAmareloEsquerdoElementbuffer);
	deleteBuffers(botaoAmareloDireitoVertexbuffer, botaoAmareloDireitoUvbuffer, botaoAmareloDireitoNormalbuffer, botaoAmareloDireitoElementbuffer);
	deleteBuffers(botaoVermelhoMeioVertexbuffer, botaoVermelhoMeioUvbuffer, botaoVermelhoMeioNormalbuffer, botaoVermelhoMeioElementbuffer);
	deleteBuffers(mesaVertexbuffer, mesaUvbuffer, mesaNormalbuffer, mesaElementbuffer);
	deleteBuffers(restoJogoVertexbuffer, restoJogoUvbuffer, restoJogoNormalbuffer, restoJogoElementbuffer);
	deleteBuffers(meioRestoJogoVertexbuffer, meioRestoJogoUvbuffer, meioRestoJogoNormalbuffer, meioRestoJogoElementbuffer);

	glDeleteProgram(programID);

	glDeleteTextures(1, &Texture_mesa_toda);
	glDeleteTextures(1, &Texture_botao_amarelo);
	glDeleteTextures(1, &Texture_botao_azul);
	glDeleteTextures(1, &Texture_botao_verde);
	glDeleteTextures(1, &Texture_botao_vermelho);
	glDeleteTextures(1, &Texture_botaozinho_direito);
	glDeleteTextures(1, &Texture_botaozinho_centro);
	glDeleteTextures(1, &Texture_botaozinho_esquerdo);
	glDeleteTextures(1, &Texture_circulo_central);
 	glDeleteTextures(1, &Texture_genius_externo);

	// Close GUI and OpenGL window, and terminate GLFW
	TwTerminate();
	glfwTerminate();
 
	return 0;
}
 