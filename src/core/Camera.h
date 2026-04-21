#pragma once

#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

namespace usim {

/// Free-fly camera with WASD + mouse look, plus focus/follow mode.
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

    /// Process scroll for speed adjustment (free-fly) or distance (follow).
    void processScroll(float yOffset);

    /// Enable follow mode: camera orbits around a target position.
    void setFollowTarget(const glm::vec3& target, float radius);

    /// Disable follow mode, return to free-fly.
    void clearFollowTarget();

    /// Update follow mode (call each frame when following).
    /// Returns true if in follow mode.
    bool updateFollow(const glm::vec3& target, float deltaTime);

    bool isFollowing() const { return m_following; }
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

    // Follow mode
    bool m_following = false;
    glm::vec3 m_followTarget = glm::vec3(0.0f);
    float m_followDistance = 10.0f;
    float m_followYaw = 0.0f;
    float m_followPitch = 20.0f;

    // Smooth transition
    bool m_transitioning = false;
    glm::vec3 m_transitionStart;
    float m_transitionTime = 0.0f;
    float m_transitionDuration = 1.0f;
};

} // namespace usim
