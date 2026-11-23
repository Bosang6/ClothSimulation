#include "app/Application.h"
#include <iostream>

namespace cloth
{

inline void PrintErr()
{
    std::cerr << "Failed to initialize\n";
}

Application::~Application()
{
    shutdown();
}

bool Application::initialize()
{
    if(m_window) return true;

    if(!glfwInit())
    {
        PrintErr();
        return false;
    }

    // Set OpenGL Version
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Create Window
    m_window = glfwCreateWindow(1280, 720, "ClothSimulatior", nullptr, nullptr);
    if(!m_window)
    {
        PrintErr();
        shutdown();
        return false;
    }

    glfwMakeContextCurrent(m_window);
    glfwSetFramebufferSizeCallback(m_window, framebufferSizeCallback);

    // GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        PrintErr();
        shutdown();
        return false;
    }

    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(m_window, &width, &height);
    glViewport(0, 0, width, height);

    m_glfwInitialized = true;
    return m_glfwInitialized;
}

void Application::run()
{
    if(!m_window) return;

    double lastTime = glfwGetTime();

    while(!glfwWindowShouldClose(m_window))
    {
        const double currentTime = glfwGetTime();
        const float deltaTime = static_cast<float>(currentTime - lastTime);

        lastTime = currentTime;

        glfwPollEvents();

        processInput();
        update(deltaTime);
        render();

        glfwSwapBuffers(m_window);
    }
}

void Application::shutdown()
{
    if(m_window)
    {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }

    if(m_glfwInitialized)
    {
        glfwTerminate();
        m_glfwInitialized = false;
    }
}

void Application::processInput()
{
    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(m_window, true);
    }
}

void Application::update(float deltaTime)
{
    // Simulation...
}

void Application::render()
{
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Application::framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

}
