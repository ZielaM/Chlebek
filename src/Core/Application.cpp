#include "Application.h"
#include "Renderer/Shader.h"
#include <stdexcept>
#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> // Do macierzy
#include <vector>
#include <random>
#include <iostream>

Application::Application() {
    Init();
}

Application::~Application() {
    Cleanup();
}

void Application::Init() {
    // 1. Inicjalizacja GLFW
    if (!glfwInit()) {
        throw std::runtime_error("Nie udało się zainicjować GLFW");
    }

    // Konfiguracja wersji OpenGL (4.1 Core Profile)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 2. Tworzenie Okna
    m_Window = glfwCreateWindow(m_Width, m_Height, "Symulator Siatki Glutenowej", nullptr, nullptr);
    if (!m_Window) {
        glfwTerminate();
        throw std::runtime_error("Nie udało się stworzyć okna GLFW");
    }
    glfwMakeContextCurrent(m_Window);

    // 3. Ładowanie funkcji OpenGL przez GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        throw std::runtime_error("Nie udało się zainicjować GLAD");
    }

    // Ustawienie Viewportu
    glViewport(0, 0, m_Width, m_Height);
    
    // Callback zmiany rozmiaru
    glfwSetFramebufferSizeCallback(m_Window, [](GLFWwindow* window, int width, int height){
        glViewport(0, 0, width, height);
    });

    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_DEPTH_TEST);

    m_SimEngine.Init(1000); // Stwórz 1000 cząsteczek (test)

    std::cout << "OpenGL Init OK! Wersja: " << glGetString(GL_VERSION) << std::endl;
}

void Application::MainLoop() {
    Shader shader("res/shaders/basic.vert", "res/shaders/basic.frag");

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    // Konfiguracja VAO (robimy to raz)
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // Rezerwujemy pamięć na starcie (DYNAMIC_DRAW bo będziemy to zmieniać co klatkę!)
    glBufferData(GL_ARRAY_BUFFER, 1000 * 3 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    float lastTime = 0.0f;

    while (!glfwWindowShouldClose(m_Window)) {
        // Obliczanie Delta Time
        float currentTime = (float)glfwGetTime();
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        // Ogranicznik dla stabilności (gdyby klatka trwała za długo)
        if (deltaTime > 0.05f) deltaTime = 0.05f;

        // --- 1. AKTUALIZACJA FIZYKI ---
        m_SimEngine.Update(deltaTime);

        // --- 2. PRZESŁANIE DANYCH DO GPU ---
        // Musimy zamienić vector<Agent> na vector<float>
        const auto& agents = m_SimEngine.GetAgents();
        std::vector<float> gpuData;
        gpuData.reserve(agents.size() * 3);
        
        for (const auto& agent : agents) {
            gpuData.push_back(agent.position.x);
            gpuData.push_back(agent.position.y);
            gpuData.push_back(agent.position.z);
        }

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        // Nadpisujemy bufor nowymi pozycjami (glBufferSubData jest szybsze niż glBufferData przy aktualizacji)
        glBufferSubData(GL_ARRAY_BUFFER, 0, gpuData.size() * sizeof(float), gpuData.data());


        // --- 3. RENDEROWANIE ---
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.Use();
        
        // Kamera (lekko oddalona)
        glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 1.0f, 4.0f), 
                                     glm::vec3(0.0f, 0.5f, 0.0f), 
                                     glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)m_Width / (float)m_Height, 0.1f, 100.0f);
        shader.SetMat4("u_ViewProjection", projection * view);
        shader.SetVec3("u_Color", glm::vec3(1.0f, 0.8f, 0.6f)); // Kolor ciasta

        glBindVertexArray(VAO);
        glDrawArrays(GL_POINTS, 0, agents.size());

        glfwSwapBuffers(m_Window);
        glfwPollEvents();
    }
    
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

void Application::Run() {
    MainLoop();
}

void Application::Cleanup() {
    if (m_Window) {
        glfwDestroyWindow(m_Window);
    }
    glfwTerminate();
}