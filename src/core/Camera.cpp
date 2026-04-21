#include "core/Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

namespace usim {

Camera::Camera(glm::vec3 position, float yaw, float pitch)
    : m_position(position), m_yaw(yaw), m_pitch(pitch)
{
    updateVectors();
}

glm::mat4 Camera::viewMatrix() const {
    if (m_following) {
        return glm::lookAt(m_position, m_followTarget, m_worldUp);
    }
    return glm::lookAt(m_position, m_position + m_front, m_up);
}

glm::mat4 Camera::projectionMatrix(float aspect) const {
    return glm::perspective(glm::radians(m_fov), aspect, m_near, m_far);
}

void Camera::processKeyboard(GLFWwindow* window, float deltaTime) {
    if (m_following) {
        float orbitSpeed = 60.0f * deltaTime;

        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            m_followYaw -= orbitSpeed;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            m_followYaw += orbitSpeed;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            m_followPitch = std::min(m_followPitch + orbitSpeed, 89.0f);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            m_followPitch = std::max(m_followPitch - orbitSpeed, -89.0f);

        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            m_followDistance = std::max(m_followDistance - m_followDistance * deltaTime, 0.5f);
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
            m_followDistance += m_followDistance * deltaTime;

        return;
    }

    float velocity = m_speed * m_speedScale * deltaTime;

    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        velocity *= 5.0f;
    }

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        m_position += m_front * velocity;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        m_position -= m_front * velocity;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        m_position -= m_right * velocity;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        m_position += m_right * velocity;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        m_position += m_worldUp * velocity;
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        m_position -= m_worldUp * velocity;
}

void Camera::processMouseMovement(float xOffset, float yOffset) {
    if (m_following) {
        m_followYaw += xOffset * m_sensitivity;
        m_followPitch += yOffset * m_sensitivity;
        m_followPitch = std::clamp(m_followPitch, -89.0f, 89.0f);
        return;
    }

    xOffset *= m_sensitivity;
    yOffset *= m_sensitivity;

    m_yaw += xOffset;
    m_pitch += yOffset;

    m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);
    updateVectors();
}

void Camera::processScroll(float yOffset) {
    if (m_following) {
        m_followDistance -= yOffset * m_followDistance * 0.1f;
        m_followDistance = std::clamp(m_followDistance, 0.5f, 10000.0f);
        return;
    }

    m_speed += yOffset * 5.0f * m_speedScale;
    m_speed = std::clamp(m_speed, 1.0f, 5000.0f);
}

void Camera::setFollowTarget(const glm::vec3& target, float radius) {
    m_following = true;
    m_followTarget = target;
    m_followDistance = radius * 4.0f;
    m_transitioning = true;
    m_transitionStart = m_position;
    m_transitionTime = 0.0f;

    glm::vec3 offset = m_position - target;
    float dist = glm::length(offset);
    if (dist > 0.001f) {
        m_followYaw = glm::degrees(std::atan2(offset.z, offset.x));
        m_followPitch = glm::degrees(std::asin(
            std::clamp(offset.y / dist, -1.0f, 1.0f)));
    }
}

void Camera::clearFollowTarget() {
    if (m_following) {
        glm::vec3 dir = glm::normalize(m_followTarget - m_position);
        m_yaw = glm::degrees(std::atan2(dir.z, dir.x));
        m_pitch = glm::degrees(std::asin(std::clamp(dir.y, -1.0f, 1.0f)));
        updateVectors();
    }
    m_following = false;
    m_transitioning = false;
}

bool Camera::updateFollow(const glm::vec3& target, float deltaTime) {
    if (!m_following) return false;

    m_followTarget = target;

    float yawRad = glm::radians(m_followYaw);
    float pitchRad = glm::radians(m_followPitch);

    glm::vec3 offset;
    offset.x = std::cos(pitchRad) * std::cos(yawRad);
    offset.y = std::sin(pitchRad);
    offset.z = std::cos(pitchRad) * std::sin(yawRad);
    offset *= m_followDistance;

    glm::vec3 desiredPos = target + offset;

    if (m_transitioning) {
        m_transitionTime += deltaTime;
        float t = std::min(m_transitionTime / m_transitionDuration, 1.0f);
        t = t * t * (3.0f - 2.0f * t);
        m_position = glm::mix(m_transitionStart, desiredPos, t);
        if (t >= 1.0f) {
            m_transitioning = false;
        }
    } else {
        m_position = glm::mix(m_position, desiredPos, std::min(deltaTime * 8.0f, 1.0f));
    }

    return true;
}

void Camera::updateVectors() {
    glm::vec3 front;
    front.x = std::cos(glm::radians(m_yaw)) * std::cos(glm::radians(m_pitch));
    front.y = std::sin(glm::radians(m_pitch));
    front.z = std::sin(glm::radians(m_yaw)) * std::cos(glm::radians(m_pitch));
    m_front = glm::normalize(front);
    m_right = glm::normalize(glm::cross(m_front, m_worldUp));
    m_up = glm::normalize(glm::cross(m_right, m_front));
}

} // namespace usim
