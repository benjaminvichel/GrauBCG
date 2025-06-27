#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GLAD/glad.h>

class Camera
{
public:
    Camera(glm::vec3 position);
    void setPosition(const glm::vec3 &pos) { m_position = pos; }
    void setLookAt(glm::vec3 lookAt) { m_lookAt = lookAt; }
    void mouseCallback(double xpos, double ypos);
    void initCamera(GLuint shaderID);
    void update(struct GLFWwindow *window);
    glm::vec3 getPosition() const { return m_position; }
    glm::vec3 getLookAt() const { return m_lookAt; }
    glm::vec3 getCameraUp() const { return m_cameraUp; }
    glm::mat4 getViewMatrix() const
    {
        return glm::lookAt(m_position, m_position + m_lookAt, m_cameraUp);
    }

private:
    void processInput(struct GLFWwindow *window);
    glm::vec3 m_position;
    glm::vec3 m_lookAt = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 m_cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

    GLint m_viewLoc;
    GLuint m_shaderID;

    // Para controle de �ngulo
    float yaw = -90.0f; // Come�a olhando no -Z
    float pitch = 0.0f;
    float lastX = 800.0f / 2.0; // Metade da largura da janela
    float lastY = 600.0f / 2.0; // Metade da altura da janela
    bool firstMouse = true;

    // Sensibilidade do mouse
    float sensitivity = 0.1f;
};
