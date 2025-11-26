#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include "Simulation/SimulationEngine.h"

// ImGui / ImPlot
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"

class Application {
public:
    Application(int width, int height, const char* title);
    ~Application();

    void Run();

private:
    void Init();
    void MainLoop();
    void Cleanup();
    void ProcessInput();
    void RenderUI(); // New UI Method

    GLFWwindow* m_Window;
    int m_Width, m_Height;
    const char* m_Title;
    
    SimulationEngine m_SimEngine;
    float m_TimeAccumulator = 0.0f;
    const float m_FixedStep = 0.01f; // Fizyka liczy się zawsze co 10ms (100 FPS)
    
    // Rendering
    unsigned int m_AgentVAO, m_AgentVBO, m_ColorVBO;
    unsigned int m_BondVAO, m_BondVBO;
    
    // UI / Plot Data
    bool m_RenderSimulation = true;
    float m_TimeScale = 1.0f;
    bool m_IsPaused = false;
    
    std::vector<float> m_PlotTime;
    std::vector<float> m_PlotBonds;
    std::vector<float> m_PlotBroken;
    std::vector<float> m_PlotYoungs;
};