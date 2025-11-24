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

    std::cout << "OpenGL Init OK! Wersja: " << glGetString(GL_VERSION) << std::endl;
}

void Application::MainLoop() {
    // 1. Stwórz Shader
    // Upewnij się, że ścieżka jest poprawna względem pliku .exe!
    Shader shader("res/shaders/basic.vert", "res/shaders/basic.frag");

    // 2. Wygeneruj losowe agenty (punkty)
    std::vector<float> vertices;
    int numAgents = 1000;
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f); // Pozycje od -1 do 1

    for(int i = 0; i < numAgents; i++) {
        vertices.push_back(dist(rng)); // X
        vertices.push_back(dist(rng)); // Y
        vertices.push_back(dist(rng)); // Z
    }

    // 3. Skonfiguruj bufory OpenGL (VAO i VBO)
    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // Mówimy OpenGL jak czytać dane: 3 floaty na wierzchołek (X, Y, Z)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Pętla główna
    while (!glfwWindowShouldClose(m_Window)) {
        if (glfwGetKey(m_Window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(m_Window, true);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // --- RENDEROWANIE ---
        shader.Use();

        // Prosta kamera "orbitująca" (automatyczny obrót)
        float time = (float)glfwGetTime();
        float radius = 3.0f;
        float camX = sin(time) * radius;
        float camZ = cos(time) * radius;

        glm::mat4 view = glm::lookAt(glm::vec3(camX, 1.0f, camZ), // Gdzie jest kamera
                                     glm::vec3(0.0f, 0.0f, 0.0f), // Na co patrzy (środek)
                                     glm::vec3(0.0f, 1.0f, 0.0f)); // Gdzie jest "góra"
        
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)m_Width / (float)m_Height, 0.1f, 100.0f);
        
        glm::mat4 mvp = projection * view;
        
        // Wyślij macierz i kolor do shadera
        shader.SetMat4("u_ViewProjection", mvp);
        shader.SetVec3("u_Color", glm::vec3(1.0f, 0.6f, 0.2f)); // Kolor ciasta

        // Narysuj punkty
        glBindVertexArray(VAO);
        glDrawArrays(GL_POINTS, 0, numAgents);

        // -------------------

        glfwSwapBuffers(m_Window);
        glfwPollEvents();
    }

    // Sprzątanie
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