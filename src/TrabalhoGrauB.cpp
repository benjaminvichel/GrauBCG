#include "Camera.h"
#include "Camera.cpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>

#include "../Common/json.hpp"
using json = nlohmann::json;

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace std;

struct Material
{
    glm::vec3 Ka;
    glm::vec3 Kd;
    glm::vec3 Ks;
    float Ns;
    std::string map_Kd;
    std::string map_Bump;
    std::string map_Ks;
};
struct AnimatedObject
{
    GLuint VAO;
    GLuint textureID;
    size_t vertexCount;
    glm::vec3 position;
    glm::vec3 rotation = glm::vec3(0.0f); // em graus
    float scale = 1.0f;
    std::vector<glm::vec3> waypoints;
    int currentWaypoint = 0;
    float t = 0.0f;
};

std::vector<AnimatedObject> objects;
int selectedObjectIndex = 0;
bool addWaypointKeyPressed = false;

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
GLuint loadGeometry(
    const std::string &objPath,
    const Material &mat,
    const std::string &texFolder,
    GLuint &outTexDiffuseID,
    GLuint &outTexNormalID,
    GLuint &outTexSpecularID,
    size_t &outVertexCount);
GLuint loadTexture(const std::string &filePath, int &width, int &height);

const GLuint WIDTH = 1000, HEIGHT = 1000;

std::vector<glm::vec3> waypoints;
int currentWaypoint = 0;
float t = 0.0f;
float speed = 0.5f;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// variaveis de iluminacao modificaveis
float ambientStrength = 1.0f;
float diffuseStrength = 1.0f;
float specularStrength = 1.0f;

glm::vec3 objectPosition = glm::vec3(0.0f); // posição do objeto que será animado

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
    
    // Luz e câmera
    uniform vec3 lightPos;
    uniform vec3 viewPos;
    uniform vec3 lightColor;
    uniform float ambientStrength;
uniform float diffuseStrength;
uniform float specularStrength;

    
    // Textura difusa
    uniform sampler2D texture1;
    
    //Uniforms do material (vindos do .mtl)
    uniform vec3 Ka;       // Ambiente
    uniform vec3 Kd;       // Difusa
    uniform vec3 Ks;       // Especular
    uniform float Ns;      // Brilho
    
    void main()
    {
        vec3 texColor = texture(texture1, fragTexCoord).rgb;
    
        // Vetores de iluminação
        vec3 norm = normalize(Normal);
        vec3 lightDir = normalize(lightPos - FragPos);
        vec3 viewDir = normalize(viewPos - FragPos);
        vec3 reflectDir = reflect(-lightDir, norm);
    
        // Componentes Phong
        float diff = max(dot(norm, lightDir), 0.0);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), Ns);
    
        // usando Ka, Kd e Ks
        vec3 ambient  = Ka * texColor * ambientStrength;
vec3 diffuse  = Kd * diff * texColor * diffuseStrength;
vec3 specular = Ks * spec * lightColor * specularStrength;

    
        vec3 result = ambient + diffuse + specular;
        color = vec4(result, 1.0);
    }
    )glsl";

Material loadMaterial(const std::string &mtlPath)
{
    // Inicializa o Material com valores padrão
    Material mat;
    mat.Ka = glm::vec3(1.0f);
    mat.Kd = glm::vec3(1.0f);
    mat.Ks = glm::vec3(1.0f);
    mat.Ns = 32.0f;
    mat.map_Kd = "";
    // Abre o arquivo .mtl
    std::ifstream mtlFile(mtlPath);
    if (!mtlFile.is_open())
    {
        std::cerr << "Erro ao abrir MTL: " << mtlPath << std::endl;
        return mat;
    }

    std::string line;
    while (getline(mtlFile, line))
    {
        std::istringstream iss(line);
        std::string type;
        iss >> type;

        if (type == "Ka")
            iss >> mat.Ka.r >> mat.Ka.g >> mat.Ka.b;
        else if (type == "Kd")
            iss >> mat.Kd.r >> mat.Kd.g >> mat.Kd.b;
        else if (type == "Ks")
            iss >> mat.Ks.r >> mat.Ks.g >> mat.Ks.b;
        else if (type == "Ns")
            iss >> mat.Ns;
        else if (type == "map_Kd")
            iss >> mat.map_Kd;

        else if (type == "map_Bump" || type == "bump")
            iss >> mat.map_Bump;
        else if (type == "map_Ks")
            iss >> mat.map_Ks;
        std::cout << "Textura difusa carregada do .mtl: " << mat.map_Kd << std::endl;
    }

    mtlFile.close();
    return mat;
}

void loadSceneFromFile(const std::string &filename, const std::string &assetPath, std::vector<Material> &materials, GLuint shaderID)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Erro ao abrir arquivo de cena: " << filename << std::endl;
        return;
    }
    // operador >> da biblioteca automaticamente faz o parsing do JSON para o objeto scene
    json scene;
    file >> scene;

    objects.clear();
    materials.clear();

    for (const auto &obj : scene["objects"])
    {
        std::string objFile = assetPath + "/" + obj["file"].get<std::string>();
        std::string mtlFile = assetPath + "/" + obj["material"].get<std::string>();

        Material mat = loadMaterial(mtlFile);
        materials.push_back(mat);

        GLuint texDiffuse = 0, texNormal = 0, texSpecular = 0;
        size_t vertexCount = 0;

        // Captura o VAO retornado
        GLuint VAO = loadGeometry(objFile, mat, assetPath, texDiffuse, texNormal, texSpecular, vertexCount);

        if (texDiffuse == 0)
            std::cerr << "Aviso: textura difusa não carregada corretamente para " << objFile << std::endl;

        AnimatedObject object;
        object.VAO = VAO;
        object.textureID = texDiffuse;
        object.vertexCount = vertexCount;
        object.position = glm::vec3(obj["position"][0], obj["position"][1], obj["position"][2]);
        object.rotation = glm::vec3(obj["rotation"][0], obj["rotation"][1], obj["rotation"][2]);
        object.scale = obj["scale"].get<float>();

        for (const auto &wp : obj["waypoints"])
            object.waypoints.push_back(glm::vec3(wp[0], wp[1], wp[2]));

        objects.push_back(object);
    }

    // Atualiza luz no shader
    glm::vec3 lightPos(
        scene["light"]["position"][0],
        scene["light"]["position"][1],
        scene["light"]["position"][2]);
    glm::vec3 lightColor(
        scene["light"]["color"][0],
        scene["light"]["color"][1],
        scene["light"]["color"][2]);
    glUseProgram(shaderID);
    glUniform3fv(glGetUniformLocation(shaderID, "lightPos"), 1, glm::value_ptr(lightPos));
    glUniform3fv(glGetUniformLocation(shaderID, "lightColor"), 1, glm::value_ptr(lightColor));

    // Atualiza posição da câmera
    if (g_camera)
    {
        g_camera->setPosition(glm::vec3(
            scene["camera"]["position"][0],
            scene["camera"]["position"][1],
            scene["camera"]["position"][2]));
    }
}
glm::vec3 catmullRom(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, float t)
{
    return 0.5f * ((2.0f * p1) +
                   (-p0 + p2) * t +
                   (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t * t +
                   (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t * t * t);
}
int main()
{

    // Atualiza posição do objeto com base nos waypoints

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
    std::vector<Material> materials;
    GLuint texID_Suzanne, texID_Cube;
    size_t vertexCount_Suzanne, vertexCount_Cube;

    std::string assetPath = "../assets/modelos3D";
    loadSceneFromFile("../assets/scene.json", assetPath, materials, shaderID);

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

    // Loop principal
    while (!glfwWindowShouldClose(window))
    {
        // Calcula deltaTime
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();
        camera.update(window);

        // Limpa tela e depth buffer
        glClearColor(0.9f, 0.9f, 0.9f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderID);

        // Atualiza view e projection
        glm::mat4 view = camera.getViewMatrix();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3fv(viewPosLoc, 1, glm::value_ptr(camera.getPosition()));

        // Atualiza posição dos objetos animados
        for (auto &obj : objects)
        {
            if (!obj.waypoints.empty())
            {
                int count = obj.waypoints.size();
                int i0 = (obj.currentWaypoint - 1 + count) % count;
                int i1 = obj.currentWaypoint;
                int i2 = (obj.currentWaypoint + 1) % count;
                int i3 = (obj.currentWaypoint + 2) % count;

                glm::vec3 p0 = obj.waypoints[i0];
                glm::vec3 p1 = obj.waypoints[i1];
                glm::vec3 p2 = obj.waypoints[i2];
                glm::vec3 p3 = obj.waypoints[i3];

                obj.position = catmullRom(p0, p1, p2, p3, obj.t);
                obj.t += speed * deltaTime;
                if (obj.t >= 1.0f)
                {
                    obj.t = 0.0f;
                    obj.currentWaypoint = (obj.currentWaypoint + 1) % obj.waypoints.size();
                }
            }
        }
        // enviam valores de intensidade de iluminação para o seu fragment shader
        glUniform1f(glGetUniformLocation(shaderID, "ambientStrength"), ambientStrength);
        glUniform1f(glGetUniformLocation(shaderID, "diffuseStrength"), diffuseStrength);
        glUniform1f(glGetUniformLocation(shaderID, "specularStrength"), specularStrength);

        // Renderiza todos os objetos
        for (size_t i = 0; i < objects.size(); ++i)
        {
            const auto &obj = objects[i];
            glm::mat4 model = glm::translate(glm::mat4(1.0f), obj.position);

            // Aplica rotações em ZYX (em graus → radianos)
            model = glm::rotate(model, glm::radians(obj.rotation.z), glm::vec3(0, 0, 1));
            model = glm::rotate(model, glm::radians(obj.rotation.y), glm::vec3(0, 1, 0));
            model = glm::rotate(model, glm::radians(obj.rotation.x), glm::vec3(1, 0, 0));

            // Aplica escala uniforme
            model = glm::scale(model, glm::vec3(obj.scale));

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

            // Passa o material do objeto para o shader
            const Material &mat = materials[i];

            GLint KaLoc = glGetUniformLocation(shaderID, "Ka");
            GLint KdLoc = glGetUniformLocation(shaderID, "Kd");
            GLint KsLoc = glGetUniformLocation(shaderID, "Ks");
            GLint NsLoc = glGetUniformLocation(shaderID, "Ns");

            glUniform3fv(KaLoc, 1, glm::value_ptr(mat.Ka));
            glUniform3fv(KdLoc, 1, glm::value_ptr(mat.Kd));
            glUniform3fv(KsLoc, 1, glm::value_ptr(mat.Ks));
            glUniform1f(NsLoc, mat.Ns);

            // fay com que cada objeto use sua texture
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, obj.textureID);
            glUniform1i(glGetUniformLocation(shaderID, "texture1"), 0);
            glBindVertexArray(obj.VAO);
            glDrawArrays(GL_TRIANGLES, 0, (GLsizei)obj.vertexCount);
            glBindVertexArray(0);
        }

        glfwSwapBuffers(window);
    }

    // Cleanup
    for (const auto &obj : objects)
    {
        glDeleteVertexArrays(1, &obj.VAO);
        glDeleteTextures(1, &obj.textureID);
    }

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

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (key == GLFW_KEY_E && action == GLFW_PRESS)
    {
        if (!addWaypointKeyPressed && g_camera)
        {
            glm::vec3 pos = g_camera->getPosition();
            objects[selectedObjectIndex].waypoints.push_back(pos);
            std::cout << "Waypoint adicionado ao objeto " << selectedObjectIndex << ": " << pos.x << ", " << pos.y << ", " << pos.z << std::endl;
            addWaypointKeyPressed = true;
        }
    }
    if (key == GLFW_KEY_E && action == GLFW_RELEASE)
    {
        addWaypointKeyPressed = false;
    }

    if (key == GLFW_KEY_TAB && action == GLFW_PRESS)
    {
        selectedObjectIndex = (selectedObjectIndex + 1) % objects.size();
        std::cout << "Objeto selecionado: " << selectedObjectIndex << std::endl;
    }

    if (key == GLFW_KEY_E && action == GLFW_PRESS)
    {
        if (!addWaypointKeyPressed && g_camera)
        {
            glm::vec3 pos = g_camera->getPosition();
            objects[selectedObjectIndex].waypoints.push_back(pos);
            std::cout << "Waypoint adicionado ao objeto " << selectedObjectIndex << ": " << pos.x << ", " << pos.y << ", " << pos.z << std::endl;
            addWaypointKeyPressed = true;
        }
    }
    if (key == GLFW_KEY_E && action == GLFW_RELEASE)
    {
        addWaypointKeyPressed = false;
    }
    if (key == GLFW_KEY_1 && action == GLFW_PRESS)
        ambientStrength = std::max(0.0f, ambientStrength - 0.1f);
    if (key == GLFW_KEY_2 && action == GLFW_PRESS)
        ambientStrength += 0.1f;

    if (key == GLFW_KEY_3 && action == GLFW_PRESS)
        diffuseStrength = std::max(0.0f, diffuseStrength - 0.1f);
    if (key == GLFW_KEY_4 && action == GLFW_PRESS)
        diffuseStrength += 0.1f;

    if (key == GLFW_KEY_5 && action == GLFW_PRESS)
        specularStrength = std::max(0.0f, specularStrength - 0.1f);
    if (key == GLFW_KEY_6 && action == GLFW_PRESS)
        specularStrength += 0.1f;
    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        AnimatedObject &obj = objects[selectedObjectIndex];

        // Rotação com as teclas R/T/Y
        if (key == GLFW_KEY_R)
            obj.rotation.x += 10.0f; // X
        if (key == GLFW_KEY_T)
            obj.rotation.y += 10.0f; // Y
        if (key == GLFW_KEY_Y)
            obj.rotation.z += 10.0f; // Z

        // Escala com U/I
        if (key == GLFW_KEY_U)
            obj.scale = std::max(0.1f, obj.scale - 0.1f);
        if (key == GLFW_KEY_I)
            obj.scale += 0.1f;
    }
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
GLuint loadGeometry(
    const std::string &objPath,
    const Material &mat,
    const std::string &texFolder,
    GLuint &outTexDiffuseID,
    GLuint &outTexNormalID,
    GLuint &outTexSpecularID,
    size_t &outVertexCount)
{
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texCoords;
    std::vector<Vertex> vertices;

    std::ifstream objFile(objPath);
    if (!objFile.is_open())
    {
        std::cerr << "Erro ao abrir OBJ: " << objPath << std::endl;
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
                int vi = 0, ti = 0, ni = 0;
                viss >> vi >> ti >> ni;

                Vertex vert;
                vert.position = positions[vi - 1];
                vert.texCoord = (ti > 0) ? texCoords[ti - 1] : glm::vec2(0.0f);
                vert.normal = (ni > 0) ? normals[ni - 1] : glm::vec3(0.0f);
                vertices.push_back(vert);
            }
        }
    }
    objFile.close();

    // Carrega as texturas (se houver)
    int w, h;
    if (!mat.map_Kd.empty())
        outTexDiffuseID = loadTexture(texFolder + "/" + mat.map_Kd, w, h);
    else
        outTexDiffuseID = 0;

    if (!mat.map_Bump.empty())
        outTexNormalID = loadTexture(texFolder + "/" + mat.map_Bump, w, h);
    else
        outTexNormalID = 0;

    if (!mat.map_Ks.empty())
        outTexSpecularID = loadTexture(texFolder + "/" + mat.map_Ks, w, h);
    else
        outTexSpecularID = 0;

    // Cria VAO e VBO
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
