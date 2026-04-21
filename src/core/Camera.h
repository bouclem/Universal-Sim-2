#pragma once

#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

namespace usim {

/// Free-fly camera with WASD + mouse look, plus focus/follow mode.
/// Movement speed auto-scales based on the current viewing scale.
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

    /// Set the movement speed scale factor (for multi-scale universe).
    /// 1.0 = solar system, ~100 = star field, ~1000 = galaxy
    void setSpeedScale(float scale) { m_speedScale = scale; }
    float speedScale() const { return m_speedScale; }

    bool isFollowing() const { return m_following; }
    glm::vec3 position() const { return m_position; }
    glm::vec3 front() const { return m_front; }
    float nearPlane() const { return m_near; }
    float farPlane() const { return m_far; }

    void setFarPlane(float far) { m_far = far; }

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

    // Multi-scale speed
    float m_speedScale = 1.0f;

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
