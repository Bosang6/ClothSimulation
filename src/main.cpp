#include <glad/glad.h>
#include <GLFW/glfw3.h>

//#include "Headers/shader_s.h"
//#include "Headers/Model.h"
//#include "Headers/Camera.h"

#include "Model.h"
#include "Camera.h"
#include "Shader_s.h"
#include "Mesh.h"
#include "Hash.h"
#include "Simulator.h"

#include <unordered_set>

#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include <string>
#define TINYFILEDIALOGS_IMPLEMENTATION
//#include "tinyfiledialogs/tinyfiledialogs.h"
#include "tinyfiledialogs.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// Camera
//Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
Camera camera(glm::vec3(0.0f, 24.0f, 45.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// frame
float deltaTime = 0.0f;
float lastFrame = 0.0f;
int frame = 0;
int frameCount = 0;
float lastTime = glfwGetTime();

// lighting
glm::vec3 lightPos = glm::vec3(0.0f, 24.0f, 45.0f);
glm::vec3 initialLightPos = glm::vec3(0.0f, 24.0f, 45.0f);

// flip texture
bool flip = true;

// models
std::vector<Model> models;

std::vector<Vertex_H*> allParticles;                    // 所有粒子（布料 + 衣架）
std::unordered_set<Vertex_H*> staticParticles;          // 记录哪些粒子是“静态”的（不可移动，如衣架）

// model size
float modelSize = 0.5f;

// start simulate
bool start = false;

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "ClothSimulatior", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    stbi_set_flip_vertically_on_load(flip);

    glEnable(GL_DEPTH_TEST);

    // init ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext(NULL);
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    Shader shader("Shaders/shader.vs", "Shaders/shader.fs");

    // default loading
    //Model ModelLoaded("Models/backpack/backpack.obj");
    //static std::string selectedFile = "Models/backpack/backpack.obj";

    //models.push_back(Model("Models/maoyi/YIFU5obj.obj"));
    models.push_back(Model("Models/maoyi/YIFU5obj.obj"));
    //models.push_back(Model("Models/maoyi/mote.obj"));
    //models.push_back(Model("Models/maoyi/YIJIA5obj.obj"));
    //Model ModelLoaded("Models/maoyi/YIFU1.obj");


     // 合并所有的顶点
    for (auto &model : models)
    {
        bool isStaticModel = (model.name == "Models/maoyi/YIJIA5obj.obj");
        std::cout << "Model name: " << model.name << std::endl;
        for (auto &mesh : model.meshes)
        {
            for (auto &vertex : mesh.vertices)
            {
                if(isStaticModel){
                    vertex.mass = 1e6f; // 静态模型的顶点质量设置为很大，避免移动
                    staticParticles.insert(&vertex);
                }

                allParticles.push_back(&vertex);
            }
        }
    }
    std::cout << "staticParticles.size(): " << staticParticles.size() << std::endl;
    // 通过所有的顶点构建哈希空间
    //Hash hash(allParticles.size());
    Hash hash(allParticles.size(), &staticParticles);
    hash.insertParticles(allParticles);
    hash.partialSum();
    hash.insertParticleMap();

    static std::string selectedFile = "Models/lino/YIFU1.obj";

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // 初始化 simulator
    Simulator simulator(allParticles, staticParticles, models, hash);

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        //deltaTime = std::min(deltaTime, 0.02f); // 限制最大步长 copilot

        // ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Create a window
        float currentTime = glfwGetTime();
        frameCount++;
        if(currentTime - lastTime >= 1.0){
            frame = frameCount;
            frameCount = 0;
            lastTime = currentTime;
        }
        
        ImGui::Begin("Preme SPAZIO per disattivare camera");

        ImGui::Text("FPS: %d", frame);

        if(!start){
            if(ImGui::Button("Start simulate")){
                start = !start;
            }
        }
        if(start){
            if(ImGui::Button("End simulate")){
                start = !start;
            }
        }

        ImGui::Text("Clicare il buttone per cambiare un'altro modello.");

        ImGui::Text("Camera Pos: (%.3f,%.3f,%.3f)", camera.Position.x, camera.Position.y, camera.Position.z);

        if (ImGui::Button("Clear All")) {
            for (size_t i = 0; i < models.size(); ++i) {
                models[i].cleanup();
            }
        }
        
        if (ImGui::Button("Add Models")) {                // Create button
            // select model
            const char* filePath = tinyfd_openFileDialog(
                "Select a File",
                "",
                0,
                NULL,
                NULL,
                0                                        // multiple selections
            );
            // add model
            if (filePath) {
                selectedFile = std::string(filePath);
                std::cout << "Selected File: " << selectedFile << std::endl;

                //ModelLoaded.cleanup();
                //ModelLoaded = Model(selectedFile);
                models.push_back(Model(selectedFile));
            }
            else {
                selectedFile = "No file selected.";
            }
        }
        
        ImGui::Text("Flipare il Texture (se neccesario)");
        if (ImGui::Button("Flip")) {
            flip = !flip;
            stbi_set_flip_vertically_on_load(flip);
            //ModelLoaded.cleanup();
            //ModelLoaded = Model(selectedFile);
        }
        ImGui::Text("Dimensione del Modello");
        ImGui::SliderFloat("Float Value", &modelSize, 0.01f, 1.0f); // ������
        ImGui::Text("Current Value: %.3f", modelSize); // ��ʾ��ǰֵ
        ImGui::End();

        processInput(window);

        // background
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // simulator
        if(start){
            //models[0].simulate(deltaTime);
            simulator.step(deltaTime);
            //simulator.substep(deltaTime);
            for(unsigned int i = 0; i < models[0].meshes.size(); i++){
                models[0].meshes[i].updateVertexPositions();
            }
            //hash.queryAndCollideAll(0.1f); copilot 说要重新建立哈希表
            // 重新构建哈希表，保证碰撞检测用的是最新粒子位置
            // hash.clear(); // 清空之前的哈希表
            // hash.insertParticles(allParticles);
            // hash.partialSum();
            // hash.insertParticleMap();
            //for(unsigned i = 0; i < 2; i++) // 迭代几次碰撞检测
            //    hash.queryAndCollideAll(0.5f); // 半径可调
        }

        // mvp

        // global light
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::rotate(model, (float)glfwGetTime(), glm::vec3(0.0f, 1.0f, 0.0f));
        lightPos = glm::vec3(model * glm::vec4(initialLightPos, 1.0f));

        shader.use();
        shader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
        shader.setVec3("lightPos", lightPos);
        //shader.setVec3("lightPos", camera.Front);
        shader.setVec3("viewPos", camera.Position);

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);

        // render the loaded model
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f)); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(modelSize, modelSize, modelSize));	// it's a bit too big for our scene, so scale it down
        shader.setMat4("model", model);

        for (size_t i = 0; i < models.size(); ++i) {
            models[i].Draw(shader);
        }
        //ModelLoaded.Draw(shader);

        // ImGui render
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();

    return 0;
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        camera.ProcessKeyboard(Camera_Movement::FORWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        camera.ProcessKeyboard(Camera_Movement::BACKWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        camera.ProcessKeyboard(Camera_Movement::LEFT, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        camera.ProcessKeyboard(Camera_Movement::RIGHT, deltaTime);
    }

}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    // enable mouse
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        return;
    }

    float xpos = (float)xposIn;
    float ypos = (float)yposIn;

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);

    lastX = xpos;
    lastY = ypos;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.ProcessMouseScroll(yoffset);
}
