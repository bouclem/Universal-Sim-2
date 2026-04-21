#pragma once

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <string>
#include <functional>

namespace usim {

/// RAII wrapper around a GLFW window.
class Window {
public:
    Window(int width, int height, const std::string& title);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool shouldClose() const;
    void pollEvents() const;
    void swapBuffers() const;

    GLFWwindow* handle() const { return m_window; }
    int width() const { return m_width; }
    int height() const { return m_height; }
    float aspectRatio() const;

    /// Callback fired on resize.
    void setResizeCallback(std::function<void(int, int)> cb);

private:
    GLFWwindow* m_window = nullptr;
    int m_width;
    int m_height;
    std::function<void(int, int)> m_resizeCallback;

    static void framebufferSizeCallback(GLFWwindow* win, int w, int h);
};

} // namespace usim
