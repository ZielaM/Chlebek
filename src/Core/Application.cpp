#include "Application.h"
#include <stdexcept>
#include <iostream>

// WAŻNE: glad musi być przed glfw
#include <glad/glad.h>
#include <GLFW/glfw3.h>

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

    std::cout << "OpenGL Init OK! Wersja: " << glGetString(GL_VERSION) << std::endl;
}

void Application::MainLoop() {
    // Pętla główna
    while (!glfwWindowShouldClose(m_Window)) {
        // Obsługa klawisza ESC
        if (glfwGetKey(m_Window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(m_Window, true);

        // --- RENDEROWANIE ---
        // Czyścimy ekran na ciemnoszary kolor (imitacja miski/tła)
        glClearColor(0.2f, 0.2f, 0.25f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // --- KONIEC RENDEROWANIA ---
        glfwSwapBuffers(m_Window);
        glfwPollEvents();
    }
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