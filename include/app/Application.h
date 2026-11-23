#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace cloth
{

class Application
{
public:
    Application() = default;
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;

    bool initialize();
    void run();
    void shutdown();

private:
    void processInput();
    void update(float deltaTime);
    void render();

    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);

    GLFWwindow* m_window = nullptr;
    bool m_glfwInitialized = false;
};

}