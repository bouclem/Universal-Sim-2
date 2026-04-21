#pragma once

#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

namespace usim {

/// Free-fly camera with WASD + mouse look.
class Camera {
public:
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 50.0f),
           float yaw = -90.0f, float pitch = 0.0f);

    glm::mat4 viewMatrix() const;
    glm::mat4 projectionMatrix(float aspect) const;

    /// Process keyboard input for movement.
    void processKeyboard(GLFWwindow* window, float deltaTime);

    /// Process mouse movement for look direction.
    void processMouseMovement(float xOffset, float yOffset);

    /// Process scroll for speed adjustment.
    void processScroll(float yOffset);

    glm::vec3 position() const { return m_position; }
    float nearPlane() const { return m_near; }
    float farPlane() const { return m_far; }

private:
    void updateVectors();

    glm::vec3 m_position;
    glm::vec3 m_front;
    glm::vec3 m_up;
    glm::vec3 m_right;
    glm::vec3 m_worldUp = glm::vec3(0.0f, 1.0f, 0.0f);

    float m_yaw;
    float m_pitch;
    float m_speed = 20.0f;
    float m_sensitivity = 0.1f;
    float m_fov = 60.0f;
    float m_near = 0.01f;
    float m_far = 100000.0f;
};

} // namespace usim
