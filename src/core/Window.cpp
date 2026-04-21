#include "core/Window.h"
#include <stdexcept>
#include <iostream>

namespace usim {

Window::Window(int width, int height, const std::string& title)
    : m_width(width), m_height(height)
{
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_SAMPLES, 4);

    m_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!m_window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(m_window);
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, framebufferSizeCallback);

    int version = gladLoadGL(glfwGetProcAddress);
    if (!version) {
        throw std::runtime_error("Failed to initialize GLAD");
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glViewport(0, 0, width, height);

    // Capture mouse for camera control
    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Store initial windowed geometry for fullscreen toggle
    m_windowedW = width;
    m_windowedH = height;
    glfwGetWindowPos(m_window, &m_windowedX, &m_windowedY);

    std::cout << "OpenGL " << GLAD_VERSION_MAJOR(version) << "."
              << GLAD_VERSION_MINOR(version) << " initialized\n";
}

Window::~Window() {
    if (m_window) {
        glfwDestroyWindow(m_window);
    }
    glfwTerminate();
}

bool Window::shouldClose() const {
    return glfwWindowShouldClose(m_window);
}

void Window::pollEvents() const {
    glfwPollEvents();
}

void Window::swapBuffers() const {
    glfwSwapBuffers(m_window);
}

float Window::aspectRatio() const {
    if (m_height == 0) return 1.0f;
    return static_cast<float>(m_width) / static_cast<float>(m_height);
}

void Window::setResizeCallback(std::function<void(int, int)> cb) {
    m_resizeCallback = std::move(cb);
}

void Window::framebufferSizeCallback(GLFWwindow* win, int w, int h) {
    glViewport(0, 0, w, h);
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(win));
    if (self) {
        self->m_width = w;
        self->m_height = h;
        if (self->m_resizeCallback) {
            self->m_resizeCallback(w, h);
        }
    }
}

void Window::toggleFullscreen() {
    if (!m_fullscreen) {
        // Save current windowed position and size
        glfwGetWindowPos(m_window, &m_windowedX, &m_windowedY);
        glfwGetWindowSize(m_window, &m_windowedW, &m_windowedH);

        // Switch to fullscreen on the current monitor
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(m_window, monitor, 0, 0,
                             mode->width, mode->height, mode->refreshRate);
    } else {
        // Restore windowed mode
        glfwSetWindowMonitor(m_window, nullptr,
                             m_windowedX, m_windowedY,
                             m_windowedW, m_windowedH, 0);
    }
    m_fullscreen = !m_fullscreen;
}

} // namespace usim
