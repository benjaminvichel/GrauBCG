#include "Camera.h"
#include "Camera.cpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace std;

struct Vertex
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texCoord;
};

Camera *g_camera = nullptr;

void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

GLuint setupShader();
GLuint loadGeometry(const std::string &objPath, const std::string &mtlPath, const std::string &texFolder, GLuint &outTexID, size_t &outVertexCount);
GLuint loadTexture(const std::string &filePath, int &width, int &height);

const GLuint WIDTH = 1000, HEIGHT = 1000;

const char *vertexShaderSource = R"glsl(
#version 450 core
layout (location = 0) in vec3 position;
layout (location = 2) in vec2 texCoord;
layout (location = 3) in vec3 normal;

out vec2 fragTexCoord;
out vec3 FragPos;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    vec4 worldPos = model * vec4(position, 1.0);
    FragPos = worldPos.xyz;
    Normal = mat3(transpose(inverse(model))) * normal;
    fragTexCoord = vec2(texCoord.x, 1 - texCoord.y);
    gl_Position = projection * view * worldPos;
}
)glsl";

const char *fragmentShaderSource = R"glsl(
#version 450 core
in vec3 FragPos;
in vec3 Normal;
in vec2 fragTexCoord;

out vec4 color;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;
uniform sampler2D texture1;

void main()
{
    vec3 texColor = texture(texture1, fragTexCoord).rgb;
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);

    vec3 ambient = 0.3 * texColor;
    vec3 diffuse = 1.2 * diff * texColor;
    vec3 specular = 0.8 * spec * lightColor;

    vec3 result = ambient + diffuse + specular;
    color = vec4(result, 1.0);
}
)glsl";

int main()
{
	// Inicializa GLFW
	if (!glfwInit())
	{
		cerr << "Failed to initialize GLFW\n";
		return -1;
	}

	GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "OpenGL OBJ Loader", nullptr, nullptr);
	if (!window)
	{
		cerr << "Failed to create GLFW window\n";
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Carrega GLAD
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		cerr << "Failed to initialize GLAD\n";
		return -1;
	}

	// Configura viewport
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);

	// Setup shader
	GLuint shaderID = setupShader();

	// Inicializa câmera
	Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
	g_camera = &camera;
	camera.initCamera(shaderID);

	// Callbacks e input
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetKeyCallback(window, key_callback);

	// Carrega modelo OBJ, MTL e textura
	GLuint VAO;
	GLuint texID;
	size_t vertexCount;
	VAO = loadGeometry("../assets/modelos3D/Suzanne.obj", "../assets/modelos3D/Suzanne.mtl", "../assets/modelos3D", texID, vertexCount);

	glEnable(GL_DEPTH_TEST);

	// Uniform locations
	GLint modelLoc = glGetUniformLocation(shaderID, "model");
	GLint viewLoc = glGetUniformLocation(shaderID, "view");
	GLint projLoc = glGetUniformLocation(shaderID, "projection");
	GLint lightPosLoc = glGetUniformLocation(shaderID, "lightPos");
	GLint viewPosLoc = glGetUniformLocation(shaderID, "viewPos");
	GLint lightColorLoc = glGetUniformLocation(shaderID, "lightColor");

	// Projeção e view (fixos para simplificar)
	glm::mat4 projection = glm::perspective(glm::radians(45.0f), float(WIDTH) / float(HEIGHT), 0.1f, 100.0f);
	glm::mat4 view = glm::lookAt(camera.getPosition(), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	// Configura shader e textura
	glUseProgram(shaderID);
	glBindTexture(GL_TEXTURE_2D, texID);
	glUniform1i(glGetUniformLocation(shaderID, "texture1"), 0);

	// Loop principal
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		camera.update(window);

		// Atualiza view e posição da câmera

		glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
		glUniform3fv(viewPosLoc, 1, glm::value_ptr(camera.getPosition()));

		// Luz
		glm::vec3 lightPos(1.2f, 1.0f, 2.0f);
		glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
		glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));
		glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

		// Limpa tela e depth buffer
		glClearColor(0.9f, 0.9f, 0.9f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Matriz model (identidade, pode rotacionar se quiser)
		glm::mat4 model = glm::mat4(1.0f);
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

		// Desenha
		glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLES, 0, (GLsizei)vertexCount);
		glBindVertexArray(0);

		glfwSwapBuffers(window);
	}

	// Cleanup
	glDeleteVertexArrays(1, &VAO);
	glDeleteProgram(shaderID);
	glfwTerminate();

	return 0;
}

// Callback do mouse para a câmera
void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
	if (g_camera)
		g_camera->mouseCallback(xpos, ypos);
}

// Callback de teclado para fechar janela
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}

// Compila e linka shader simples de vértice + fragmento
GLuint setupShader()
{
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	GLint success;
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		char infoLog[512];
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		cerr << "Erro vertex shader: " << infoLog << endl;
	}

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		char infoLog[512];
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		cerr << "Erro fragment shader: " << infoLog << endl;
	}

	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success)
	{
		char infoLog[512];
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		cerr << "Erro link shader: " << infoLog << endl;
	}

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return shaderProgram;
}

// Carrega OBJ + MTL (apenas para map_Kd) e cria VAO
GLuint loadGeometry(const std::string &objPath, const std::string &mtlPath, const std::string &texFolder, GLuint &outTexID, size_t &outVertexCount)
{
	std::vector<glm::vec3> positions;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec2> texCoords;
	std::vector<Vertex> vertices;

	std::ifstream objFile(objPath);
	if (!objFile.is_open())
	{
		cerr << "Erro ao abrir OBJ: " << objPath << endl;
		return 0;
	}

	std::string line;
	while (getline(objFile, line))
	{
		std::istringstream iss(line);
		std::string type;
		iss >> type;

		if (type == "v")
		{
			glm::vec3 pos;
			iss >> pos.x >> pos.y >> pos.z;
			positions.push_back(pos);
		}
		else if (type == "vt")
		{
			glm::vec2 uv;
			iss >> uv.x >> uv.y;
			texCoords.push_back(uv);
		}
		else if (type == "vn")
		{
			glm::vec3 norm;
			iss >> norm.x >> norm.y >> norm.z;
			normals.push_back(norm);
		}
		else if (type == "f")
		{
			for (int i = 0; i < 3; i++)
			{
				std::string v;
				iss >> v;
				std::replace(v.begin(), v.end(), '/', ' ');
				std::istringstream viss(v);
				int vi, ti, ni;
				viss >> vi >> ti >> ni;

				Vertex vert;
				vert.position = positions[vi - 1];
				vert.texCoord = texCoords[ti - 1];
				vert.normal = normals[ni - 1];
				vertices.push_back(vert);
			}
		}
	}
	objFile.close();

	// Carrega textura do arquivo MTL (procura map_Kd)
	std::ifstream mtlFile(mtlPath);
	std::string texName;
	while (getline(mtlFile, line))
	{
		std::istringstream iss(line);
		std::string type;
		iss >> type;
		if (type == "map_Kd")
		{
			iss >> texName;
			break;
		}
	}
	mtlFile.close();

	int w, h;
	outTexID = loadTexture(texFolder + "/" + texName, w, h);

	GLuint VBO, VAO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

	// position (location = 0)
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, position));
	glEnableVertexAttribArray(0);

	// texCoord (location = 2)
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, texCoord));
	glEnableVertexAttribArray(2);

	// normal (location = 3)
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, normal));
	glEnableVertexAttribArray(3);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	outVertexCount = vertices.size();

	return VAO;
}

GLuint loadTexture(const std::string &filePath, int &width, int &height)
{
	GLuint texID;
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	int nrChannels;
	unsigned char *data = stbi_load(filePath.c_str(), &width, &height, &nrChannels, 0);
	if (data)
	{
		GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		cerr << "Falha ao carregar textura: " << filePath << endl;
	}
	stbi_image_free(data);
	glBindTexture(GL_TEXTURE_2D, 0);

	return texID;
}
