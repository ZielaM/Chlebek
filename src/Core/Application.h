#pragma once
#include "../Simulation/SimulationEngine.h"

// Forward declaration (żeby nie wrzucać tutaj całego GLFW)
struct GLFWwindow;

class Application {
public:
    Application();
    ~Application();

    void Run();

private:
    void Init();
    void MainLoop();
    void Cleanup();

    GLFWwindow* m_Window = nullptr;
    int m_Width = 1280;
    int m_Height = 720;

    SimulationEngine m_SimEngine;
};