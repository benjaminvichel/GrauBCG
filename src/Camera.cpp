#include "Camera.h"
#include <GLFW/glfw3.h>

Camera::Camera(glm::vec3 position) : m_position(position)
{
}

void Camera::mouseCallback(double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // Invertido porque as coordenadas y do mouse s�o de cima para baixo

    lastX = xpos;
    lastY = ypos;

    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    // Limita o pitch para n�o virar de cabe�a para baixo
    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    m_lookAt.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    m_lookAt.y = sin(glm::radians(pitch));
    m_lookAt.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    m_lookAt = glm::normalize(m_lookAt);
}

void Camera::initCamera(GLuint shaderID)
{
    GLint camPosLoc = glGetUniformLocation(shaderID, "camPos");
    glUniform3fv(camPosLoc, 1, glm::value_ptr(m_position));

    GLint projectionLoc = glGetUniformLocation(shaderID, "projection");
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)1000 / (float)1000, 0.1f, 100.0f);
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    m_viewLoc = glGetUniformLocation(shaderID, "view");
    glm::mat4 view = glm::lookAt(
        m_position, // posi��o da c�mera
        m_lookAt,   // ponto para onde olha
        m_cameraUp  // eixo "up" (para cima)
    );
    glUniformMatrix4fv(m_viewLoc, 1, GL_FALSE, glm::value_ptr(view));
}

void Camera::update(GLFWwindow *window)
{
    processInput(window);
    glm::mat4 view = glm::lookAt(m_position, m_position + m_lookAt, m_cameraUp);
    glUniformMatrix4fv(m_viewLoc, 1, GL_FALSE, glm::value_ptr(view));
}

void Camera::processInput(GLFWwindow *window)
{
    const float speed = 0.01f;

    glm::vec3 Right = glm::normalize(glm::cross(m_lookAt, m_cameraUp));
    glm::vec3 Up = glm::normalize(glm::cross(Right, m_lookAt));

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        m_position += m_lookAt * speed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        m_position -= m_lookAt * speed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        m_position -= Right * speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        m_position += Right * speed;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        m_position += Up * speed;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        m_position -= Up * speed;
}