/* Hello Triangle - código adaptado de https://learnopengl.com/#!Getting-started/Hello-Triangle
 *
 * Adaptado por Rossana Baptista Queiroz
 * para as disciplinas de Processamento Gráfico/Computação Gráfica - Unisinos
 * Versão inicial: 7/4/2017
 * Última atualização em 07/03/2025
 */

#include <iostream>
#include <string>
#include <assert.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <algorithm> // para std::replace

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// Protótipo da função de callback de teclado
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);

struct Vertex
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texCoord;
	glm::vec3 color;
};
// Protótipos das funções
int setupShader();
int setupGeometry();
GLuint loadGeometry(const std::string &objPath, const std::string &mtlPath, const std::string &texFolder, GLuint &outTexID, size_t &outVertexCount);

GLuint loadTexture(std::string filePath, int &width, int &height);

// Dimensões da janela (pode ser alterado em tempo de execução)
const GLuint WIDTH = 1000, HEIGHT = 1000;

// Código fonte do Vertex Shader (em GLSL): ainda hardcoded
const GLchar *vertexShaderSource = "#version 450\n"
								   "layout (location = 0) in vec3 position;\n"
								   "layout (location = 1) in vec3 color;\n"
								   "layout (location = 2) in vec2 texCoord;\n"
								   "layout (location = 3) in vec3 normal;\n"

								   "out vec3 fragColor;\n"
								   "out vec2 fragTexCoord;\n"
								   "out vec3 FragPos;\n"
								   "out vec3 Normal;\n"

								   "uniform mat4 model;\n"
								   "uniform mat4 view;\n"
								   "uniform mat4 projection;\n"

								   "void main()\n"
								   "{\n"
								   "    vec4 worldPos = model * vec4(position, 1.0);\n"
								   "    FragPos = worldPos.xyz;\n"
								   "    Normal = mat3(transpose(inverse(model))) * normal;\n"
								   "    fragColor = color;\n"
								   "    fragTexCoord = vec2(texCoord.x, 1 - texCoord.y);\n"
								   "    gl_Position = projection * view * worldPos;\n"
								   "}\0";

// Códifo fonte do Fragment Shader (em GLSL): ainda hardcoded
const GLchar *fragmentShaderSource = "#version 450\n"
									 "in vec3 FragPos;\n"
									 "in vec3 Normal;\n"
									 "in vec3 fragColor;\n"
									 "in vec2 fragTexCoord;\n"

									 "out vec4 color;\n"

									 "uniform vec3 lightPos;\n"
									 "uniform vec3 viewPos;\n"
									 "uniform vec3 lightColor;\n"
									 "uniform sampler2D texture1;\n"

									 "void main()\n"
									 "{\n"
									 "    // Propriedades do material\n"
									 "    vec3 texColor = texture(texture1, fragTexCoord).rgb;\n"

									 "    // Normal normalizada\n"
									 "    vec3 norm = normalize(Normal);\n"
									 "    // Vetor da luz\n"
									 "    vec3 lightDir = normalize(lightPos - FragPos);\n"

									 "    // Difusa\n"
									 "    float diff = max(dot(norm, lightDir), 0.0);\n"

									 "    // Especular\n"
									 "    vec3 viewDir = normalize(viewPos - FragPos);\n"
									 "    vec3 reflectDir = reflect(-lightDir, norm);\n"
									 "    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);\n"

									 "    // Componentes de luz\n"
									 "    vec3 ambient = 0.3 * texColor;\n"
									 "    vec3 diffuse = 1.2 *diff * texColor;\n"
									 "    vec3 specular = 0.8* spec * lightColor;\n"

									 "    vec3 result = ambient + diffuse + specular;\n"

									 "    color = vec4(result, 1.0);\n"
									 "}\0";

bool rotateX = false, rotateY = false, rotateZ = false;

// Função MAIN
int main()
{
	// Inicialização da GLFW
	glfwInit();

	// Muita atenção aqui: alguns ambientes não aceitam essas configurações
	// Você deve adaptar para a versão do OpenGL suportada por sua placa
	// Sugestão: comente essas linhas de código para desobrir a versão e
	// depois atualize (por exemplo: 4.5 com 4 e 5)
	// glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	// glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	// glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Essencial para computadores da Apple
	// #ifdef __APPLE__
	//	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	// #endif

	// Criação da janela GLFW
	GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Ola 3D -- Benjamin!", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	// Fazendo o registro da função de callback para a janela GLFW
	glfwSetKeyCallback(window, key_callback);

	// GLAD: carrega todos os ponteiros d funções da OpenGL
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
	}

	// Obtendo as informações de versão
	const GLubyte *renderer = glGetString(GL_RENDERER); /* get renderer string */
	const GLubyte *version = glGetString(GL_VERSION);	/* version as a string */
	cout << "Renderer: " << renderer << endl;
	cout << "OpenGL version supported " << version << endl;

	// Definindo as dimensões da viewport com as mesmas dimensões da janela da aplicação
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);

	// Compilando e buildando o programa de shader
	GLuint shaderID = setupShader();

	GLuint VAO;
	GLuint texID;
	size_t vertexCount = 0;
	VAO = loadGeometry("../assets/modelos3D/Suzanne.obj", "../assets/modelos3D/Suzanne.mtl", "../assets/modelos3D", texID, vertexCount);

	glUseProgram(shaderID);
	glBindTexture(GL_TEXTURE_2D, texID);						// pertence a texture  ser mostrada
	glUniform1i(glGetUniformLocation(shaderID, "texture1"), 0); // slot 0

	glm::mat4 model = glm::mat4(1); // matriz identidade;
	GLint modelLoc = glGetUniformLocation(shaderID, "model");
	//
	model = glm::rotate(model, /*(GLfloat)glfwGetTime()*/ glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glEnable(GL_DEPTH_TEST);

	glm::mat4 projection = glm::perspective(
		glm::radians(45.0f),
		(float)WIDTH / (float)HEIGHT,
		0.1f, 100.0f);

	// iluminacao
	glm::mat4 view = glm::lookAt(
		glm::vec3(2.0f, 2.0f, 2.0f), // posição da câmera
		glm::vec3(0.0f, 0.0f, 0.0f), // olhando para a origem
		glm::vec3(0.0f, 1.0f, 0.0f)	 // up vector
	);

	GLint viewLoc = glGetUniformLocation(shaderID, "view");
	GLint projLoc = glGetUniformLocation(shaderID, "projection");

	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	glm::vec3 lightPos(1.2f, 1.0f, 2.0f);
	glm::vec3 viewPos(2.0f, 2.0f, 2.0f); // mesma posição da câmera usada na view
	glm::vec3 lightColor(1.0f, 1.0f, 1.0f);

	GLint lightPosLoc = glGetUniformLocation(shaderID, "lightPos");
	GLint viewPosLoc = glGetUniformLocation(shaderID, "viewPos");
	GLint lightColorLoc = glGetUniformLocation(shaderID, "lightColor");

	// Loop da aplicação - "game loop"
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(shaderID); // IMPORTANTE!

		// Atualiza angulação e rotação
		float angle = (GLfloat)glfwGetTime();
		model = glm::mat4(1);
		if (rotateX)
			model = glm::rotate(model, angle, glm::vec3(1, 0, 0));
		else if (rotateY)
			model = glm::rotate(model, angle, glm::vec3(0, 1, 0));
		else if (rotateZ)
			model = glm::rotate(model, angle, glm::vec3(0, 0, 1));

		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
		glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));
		glUniform3fv(viewPosLoc, 1, glm::value_ptr(viewPos));
		glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texID);
		glBindVertexArray(VAO);

		glDrawArrays(GL_TRIANGLES, 0, vertexCount);

		glBindVertexArray(0);
		glfwSwapBuffers(window);
	}

	// Pede pra OpenGL desalocar os buffers
	glDeleteVertexArrays(1, &VAO);
	// Finaliza a execução da GLFW, limpando os recursos alocados por ela
	glfwTerminate();

	glUseProgram(shaderID);
	glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));
	glUniform3fv(viewPosLoc, 1, glm::value_ptr(viewPos));
	glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

	return 0;
}

// Função de callback de teclado - só pode ter uma instância (deve ser estática se
// estiver dentro de uma classe) - É chamada sempre que uma tecla for pressionada
// ou solta via GLFW
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (key == GLFW_KEY_X && action == GLFW_PRESS)
	{
		rotateX = true;
		rotateY = false;
		rotateZ = false;
	}

	if (key == GLFW_KEY_Y && action == GLFW_PRESS)
	{
		rotateX = false;
		rotateY = true;
		rotateZ = false;
	}

	if (key == GLFW_KEY_Z && action == GLFW_PRESS)
	{
		rotateX = false;
		rotateY = false;
		rotateZ = true;
	}
}

// Esta função está basntante hardcoded - objetivo é compilar e "buildar" um programa de
//  shader simples e único neste exemplo de código
//  O código fonte do vertex e fragment shader está nos arrays vertexShaderSource e
//  fragmentShader source no iniçio deste arquivo
//  A função retorna o identificador do programa de shader
int setupShader()
{
	// Vertex shader
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	// Checando erros de compilação (exibição via log no terminal)
	GLint success;
	GLchar infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
				  << infoLog << std::endl;
	}
	// Fragment shader
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	// Checando erros de compilação (exibição via log no terminal)
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
				  << infoLog << std::endl;
	}
	// Linkando os shaders e criando o identificador do programa de shader
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	// Checando por erros de linkagem
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
				  << infoLog << std::endl;
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return shaderProgram;
}

GLuint loadGeometry(const std::string &objPath, const std::string &mtlPath, const std::string &texFolder, GLuint &outTexID, size_t &outVertexCount)

{
	std::vector<glm::vec3> positions;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec2> texCoords;
	std::vector<Vertex> vertices;

	std::ifstream objFile(objPath);
	std::string line;
	std::string texFile;

	while (std::getline(objFile, line))
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
				std::istringstream vss(v);
				int vi, ti, ni;
				vss >> vi >> ti >> ni;

				Vertex vert;
				vert.position = positions[vi - 1];
				vert.texCoord = texCoords[ti - 1];
				vert.normal = normals[ni - 1];
				vertices.push_back(vert);
			}
		}
		else if (type == "mtllib")
		{
			iss >> texFile; // assume que mtl e a textura vão ser lidos depois
		}
	}
	objFile.close();

	// carrega .mtl (somente Kd e map_Kd por enquanto)
	std::ifstream mtlFile(mtlPath);
	std::string texName;
	while (std::getline(mtlFile, line))
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

	// Envia para o VBO/VAO
	GLuint VBO, VAO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

	// posição
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)0);
	glEnableVertexAttribArray(0);
	// normal
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)(offsetof(Vertex, normal)));
	glEnableVertexAttribArray(3);
	// texCoord
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)(offsetof(Vertex, texCoord)));
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	outVertexCount = vertices.size();

	return VAO;
}

// Esta função está bastante harcoded - objetivo é criar os buffers que armazenam a
// geometria de um triângulo
// Apenas atributo coordenada nos vértices
// 1 VBO com as coordenadas, VAO com apenas 1 ponteiro para atributo
// A função retorna o identificador do VAO
int setupGeometry()
{
	// Aqui setamos as coordenadas x, y e z do triângulo e as armazenamos de forma
	// sequencial, já visando mandar para o VBO (Vertex Buffer Objects)
	// Cada atributo do vértice (coordenada, cores, coordenadas de textura, normal, etc)
	// Pode ser arazenado em um VBO único ou em VBOs separados
	GLfloat vertices[] = {
		// Face inferior (base) – normal: (0, -1, 0)
		-0.5f, -0.5f, -0.5f, 1, 0, 0, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, // A
		0.5f, -0.5f, -0.5f, 1, 0, 0, 1.0f, 0.0f, 0.0f, -1.0f, 0.0f,	 // B
		0.5f, -0.5f, 0.5f, 1, 0, 0, 1.0f, 1.0f, 0.0f, -1.0f, 0.0f,	 // C

		-0.5f, -0.5f, -0.5f, 1, 0, 0, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, // A
		0.5f, -0.5f, 0.5f, 1, 0, 0, 1.0f, 1.0f, 0.0f, -1.0f, 0.0f,	 // C
		-0.5f, -0.5f, 0.5f, 1, 0, 0, 0.0f, 1.0f, 0.0f, -1.0f, 0.0f,	 // D

		// Face superior (topo) – normal: (0, +1, 0)
		-0.5f, 0.5f, -0.5f, 0, 1, 0, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, // E
		0.5f, 0.5f, 0.5f, 0, 1, 0, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f,   // G
		0.5f, 0.5f, -0.5f, 0, 1, 0, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,  // F

		-0.5f, 0.5f, -0.5f, 0, 1, 0, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, // E
		-0.5f, 0.5f, 0.5f, 0, 1, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,  // H
		0.5f, 0.5f, 0.5f, 0, 1, 0, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f,   // G

		// Face frontal – normal: (0, 0, +1)
		-0.5f, -0.5f, 0.5f, 0, 0, 1, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, // D
		0.5f, -0.5f, 0.5f, 0, 0, 1, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,  // C
		0.5f, 0.5f, 0.5f, 0, 0, 1, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f,   // G

		-0.5f, -0.5f, 0.5f, 0, 0, 1, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, // D
		0.5f, 0.5f, 0.5f, 0, 0, 1, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f,   // G
		-0.5f, 0.5f, 0.5f, 0, 0, 1, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,  // H

		// Face traseira – normal: (0, 0, -1)
		-0.5f, -0.5f, -0.5f, 1, 1, 0, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, // A
		0.5f, 0.5f, -0.5f, 1, 1, 0, 1.0f, 1.0f, 0.0f, 0.0f, -1.0f,	 // F
		0.5f, -0.5f, -0.5f, 1, 1, 0, 1.0f, 0.0f, 0.0f, 0.0f, -1.0f,	 // B

		-0.5f, -0.5f, -0.5f, 1, 1, 0, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, // A
		-0.5f, 0.5f, -0.5f, 1, 1, 0, 0.0f, 1.0f, 0.0f, 0.0f, -1.0f,	 // E
		0.5f, 0.5f, -0.5f, 1, 1, 0, 1.0f, 1.0f, 0.0f, 0.0f, -1.0f,	 // F

		// Face esquerda – normal: (-1, 0, 0)
		-0.5f, -0.5f, -0.5f, 1, 0, 1, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, // A
		-0.5f, -0.5f, 0.5f, 1, 0, 1, 1.0f, 0.0f, -1.0f, 0.0f, 0.0f,	 // D
		-0.5f, 0.5f, 0.5f, 1, 0, 1, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f,	 // H

		-0.5f, -0.5f, -0.5f, 1, 0, 1, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, // A
		-0.5f, 0.5f, 0.5f, 1, 0, 1, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f,	 // H
		-0.5f, 0.5f, -0.5f, 1, 0, 1, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f,	 // E

		// Face direita – normal: (+1, 0, 0)
		0.5f, -0.5f, -0.5f, 0, 1, 1, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // B
		0.5f, 0.5f, 0.5f, 0, 1, 1, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f,   // G
		0.5f, -0.5f, 0.5f, 0, 1, 1, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,  // C

		0.5f, -0.5f, -0.5f, 0, 1, 1, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // B
		0.5f, 0.5f, -0.5f, 0, 1, 1, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f,  // F
		0.5f, 0.5f, 0.5f, 0, 1, 1, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f,   // G
	};

	GLuint VBO, VAO;

	// Geração do identificador do VBO
	glGenBuffers(1, &VBO);

	// Faz a conexão (vincula) do buffer como um buffer de array
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	// Envia os dados do array de floats para o buffer da OpenGl
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// Geração do identificador do VAO (Vertex Array Object)
	glGenVertexArrays(1, &VAO);

	// Vincula (bind) o VAO primeiro, e em seguida  conecta e seta o(s) buffer(s) de vértices
	// e os ponteiros para os atributos
	glBindVertexArray(VAO);

	// Para cada atributo do vertice, criamos um "AttribPointer" (ponteiro para o atributo), indicando:
	//  Localização no shader * (a localização dos atributos devem ser correspondentes no layout especificado no vertex shader)
	//  Numero de valores que o atributo tem (por ex, 3 coordenadas xyz)
	//  Tipo do dado
	//  Se está normalizado (entre zero e um)
	//  Tamanho em bytes
	//  Deslocamento a partir do byte zero

	// posição
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid *)0);
	// cor
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid *)(3 * sizeof(GLfloat)));
	// textura
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid *)(6 * sizeof(GLfloat)));
	// normal
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid *)(8 * sizeof(GLfloat)));

	glEnableVertexAttribArray(0); // posição
	glEnableVertexAttribArray(1); // cor
	glEnableVertexAttribArray(2); // texCoord
	glEnableVertexAttribArray(3); // normal (já está)

	// Observe que isso é permitido, a chamada para glVertexAttribPointer registrou o VBO como o objeto de buffer de vértice
	// atualmente vinculado - para que depois possamos desvincular com segurança
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Desvincula o VAO (é uma boa prática desvincular qualquer buffer ou array para evitar bugs medonhos)
	glBindVertexArray(0);

	return VAO;
}

GLuint loadTexture(string filePath, int &width, int &height)
{
	GLuint texID;
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);

	// Ajuste dos parâmetros de wrapping e filtering
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	int nrChannels;
	unsigned char *data = stbi_load(filePath.c_str(), &width, &height, &nrChannels, 0);

	if (data)
	{
		if (nrChannels == 3) // jpg, bmp
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		}
		else if (nrChannels == 4) // png com alpha
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		}
		else
		{
			std::cout << "Unsupported number of channels: " << nrChannels << std::endl;
		}
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture " << filePath << std::endl;
	}

	stbi_image_free(data);
	glBindTexture(GL_TEXTURE_2D, 0);

	return texID;
}
