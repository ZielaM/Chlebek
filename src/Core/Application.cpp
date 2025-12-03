#include "Application.h"
#include "Renderer/Shader.h"
#include <stdexcept>
#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> // For matrix operations
#include <vector>
#include <random>
#include <iostream>

Application::Application(int width, int height, const char* title) 
    : m_Width(width), m_Height(height), m_Title(title) {
    Init();
}

Application::~Application() {
    Cleanup();
}

void Application::Init() {
    // 1. Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        exit(-1);
    }

    // Configure OpenGL Version (4.5 Core Profile)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 2. Create Window
    m_Window = glfwCreateWindow(m_Width, m_Height, m_Title, NULL, NULL);
    if (!m_Window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        exit(-1);
    }
    glfwMakeContextCurrent(m_Window);

    // 3. Load OpenGL functions via GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        exit(-1);
    }

    // Set Viewport
    glViewport(0, 0, m_Width, m_Height);
    
    // Resize Callback
    glfwSetFramebufferSizeCallback(m_Window, [](GLFWwindow* window, int width, int height){
        glViewport(0, 0, width, height);
    });

    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_DEPTH_TEST);

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(m_Window, true);
    ImGui_ImplOpenGL3_Init("#version 450");

    // Simulation Init
    m_SimEngine.Init(1000); // 1000 agents

    std::cout << "OpenGL Init OK! Version: " << glGetString(GL_VERSION) << std::endl;
}

void Application::MainLoop() {
    Shader shader("res/shaders/basic.vert", "res/shaders/basic.frag");

    float lastFrame = 0.0f;

    // VBO for points (Simple rendering)
    glGenVertexArrays(1, &m_AgentVAO);
    glGenBuffers(1, &m_AgentVBO);
    glBindVertexArray(m_AgentVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_AgentVBO);
    glBufferData(GL_ARRAY_BUFFER, 1000 * 3 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // VBO for Colors
    glGenBuffers(1, &m_ColorVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_ColorVBO);
    glBufferData(GL_ARRAY_BUFFER, 1000 * 3 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    
    // VBO for Bonds (Lines)
    glGenVertexArrays(1, &m_BondVAO);
    glGenBuffers(1, &m_BondVBO);
    glGenBuffers(1, &m_BondColorVBO);
    glBindVertexArray(m_BondVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_BondVBO);
    // Estimate max bonds: 1000 agents * 4 bonds * 2 points * 3 floats
    glBufferData(GL_ARRAY_BUFFER, 1000 * 8 * 3 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Color for Bonds
    glBindBuffer(GL_ARRAY_BUFFER, m_BondColorVBO);
    glBufferData(GL_ARRAY_BUFFER, 1000 * 8 * 3 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);

    while (!glfwWindowShouldClose(m_Window)) {
        float currentFrame = glfwGetTime();
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Prevent Death Spiral (Lag)
        if (deltaTime > 0.1f) deltaTime = 0.1f;

        // Start UI Frame (Input Processing)
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Render UI (This updates variables immediately based on user input)
        RenderUI();

        // Physics Update (Fixed Step)
        // Now uses the updated variables from this frame's UI
        if (!m_IsPaused) {
            m_TimeAccumulator += deltaTime * m_TimeScale; // Apply Time Scale
            int steps = 0;
            while (m_TimeAccumulator >= m_FixedStep && steps < 5) { // Max 5 steps per frame
                m_SimEngine.Update(m_FixedStep);
                m_TimeAccumulator -= m_FixedStep;
                steps++;
                
                // Collect Plot Data
                static int plotCounter = 0;
                if (++plotCounter % 10 == 0) {
                    m_PlotTime.push_back(currentFrame);
                    m_PlotBonds.push_back((float)m_SimEngine.m_Springs.size());
                    m_PlotBroken.push_back((float)m_SimEngine.m_BrokenBondsTotal);
                    m_PlotYoungs.push_back(m_SimEngine.GetYoungsModulus());
                    
                    if (m_PlotTime.size() > 1000) {
                        m_PlotTime.erase(m_PlotTime.begin());
                        m_PlotBonds.erase(m_PlotBonds.begin());
                        m_PlotBroken.erase(m_PlotBroken.begin());
                        m_PlotYoungs.erase(m_PlotYoungs.begin());
                    }
                }
            }
        }

        ProcessInput();

        // Render Scene
        if (m_RenderSimulation) {
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            shader.Use();
            
            // Camera (slightly zoomed out)
            glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 2.0f, 5.0f), 
                                         glm::vec3(0.0f, 0.0f, 0.0f), 
                                         glm::vec3(0.0f, 1.0f, 0.0f));
            glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)m_Width / (float)m_Height, 0.1f, 100.0f);
            shader.SetMat4("u_ViewProjection", projection * view);
            
            // 1. Render Bonds (Lines)
            // We draw bonds FIRST so they are behind agents
            const auto& springs = m_SimEngine.m_Springs;
            if (!springs.empty()) {
                static std::vector<float> bondPos;
                static std::vector<float> bondColor;
                bondPos.clear();
                bondColor.clear();
                bondPos.reserve(springs.size() * 2 * 3);
                bondColor.reserve(springs.size() * 2 * 3);
                
                const float whiteColor[3] = {1.0f, 1.0f, 1.0f};
                
                for (const auto& s : springs) {
                    // Position data
                    bondPos.push_back(s.a->position.x);
                    bondPos.push_back(s.a->position.y);
                    bondPos.push_back(s.a->position.z);
                    
                    bondPos.push_back(s.b->position.x);
                    bondPos.push_back(s.b->position.y);
                    bondPos.push_back(s.b->position.z);
                    
                    // Color data (white for both vertices)
                    bondColor.insert(bondColor.end(), whiteColor, whiteColor + 3);
                    bondColor.insert(bondColor.end(), whiteColor, whiteColor + 3);
                }
                
                glBindVertexArray(m_BondVAO);
                
                glBindBuffer(GL_ARRAY_BUFFER, m_BondVBO);
                glBufferSubData(GL_ARRAY_BUFFER, 0, bondPos.size() * sizeof(float), bondPos.data());
                
                glBindBuffer(GL_ARRAY_BUFFER, m_BondColorVBO);
                glBufferSubData(GL_ARRAY_BUFFER, 0, bondColor.size() * sizeof(float), bondColor.data());
                
                glDrawArrays(GL_LINES, 0, bondPos.size() / 3);
            }
            
            // 2. Render Agents
            const auto& agents = m_SimEngine.GetAgents();
            
            static std::vector<float> gpuPos;
            static std::vector<float> gpuColor;
            gpuPos.clear();
            gpuColor.clear();
            gpuPos.reserve(agents.size() * 3);
            gpuColor.reserve(agents.size() * 3);
            
            for (const auto& agent : agents) {
                gpuPos.push_back(agent.position.x);
                gpuPos.push_back(agent.position.y);
                gpuPos.push_back(agent.position.z);
                
                // Color by Type
                switch (agent.type) {
                    case STARCH: // White
                        gpuColor.insert(gpuColor.end(), {0.9f, 0.9f, 0.9f});
                        break;
                    case GLUTENIN: // Orange
                        gpuColor.insert(gpuColor.end(), {1.0f, 0.6f, 0.0f});
                        break;
                    case GLIADIN: // Yellow
                        gpuColor.insert(gpuColor.end(), {1.0f, 0.9f, 0.2f});
                        break;
                }
            }
            
            glBindBuffer(GL_ARRAY_BUFFER, m_AgentVBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, gpuPos.size() * sizeof(float), gpuPos.data());
            
            glBindBuffer(GL_ARRAY_BUFFER, m_ColorVBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, gpuColor.size() * sizeof(float), gpuColor.data());

            glBindVertexArray(m_AgentVAO);
            shader.SetVec3("u_Color", glm::vec3(1.0f));
            glDrawArrays(GL_POINTS, 0, agents.size());
            
            // 3. Render Mixer
            float mixerPos[] = { m_SimEngine.m_Mixer.position.x, 0.0f, m_SimEngine.m_Mixer.position.z };
            glBindBuffer(GL_ARRAY_BUFFER, m_AgentVBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, 3 * sizeof(float), mixerPos);
            shader.SetVec3("u_Color", glm::vec3(1.0f, 0.0f, 0.0f)); // Red Mixer
            glPointSize(20.0f);
            glDrawArrays(GL_POINTS, 0, 1);
            glPointSize(1.0f);
            
            // 4. Render Container (Wireframe)
            std::vector<float> containerPts;
            int segments = 64;
            float r = m_SimEngine.m_ContainerRadius;
            for (int i = 0; i <= segments; ++i) {
                float theta = 2.0f * 3.14159f * float(i) / float(segments);
                containerPts.push_back(r * cos(theta));
                containerPts.push_back(-1.0f); // Floor
                containerPts.push_back(r * sin(theta));
            }
            // --- Render Lid (Top Circle) ---
            // We draw the top rim of the container at m_ContainerHeight
             for (int i = 0; i <= segments; ++i) {
                float theta = 2.0f * 3.14159f * float(i) / float(segments);
                containerPts.push_back(r * cos(theta));
                containerPts.push_back(m_SimEngine.m_ContainerHeight); // Top (Lid)
                containerPts.push_back(r * sin(theta));
            }
            
            // --- Render Lid Cross ---
            // Visual aid to help the user see the exact height of the lid
            containerPts.push_back(-r); containerPts.push_back(m_SimEngine.m_ContainerHeight); containerPts.push_back(0.0f);
            containerPts.push_back(r); containerPts.push_back(m_SimEngine.m_ContainerHeight); containerPts.push_back(0.0f);
            containerPts.push_back(0.0f); containerPts.push_back(m_SimEngine.m_ContainerHeight); containerPts.push_back(-r);
            containerPts.push_back(0.0f); containerPts.push_back(m_SimEngine.m_ContainerHeight); containerPts.push_back(r);
            
            glBindBuffer(GL_ARRAY_BUFFER, m_AgentVBO);
            glBufferData(GL_ARRAY_BUFFER, containerPts.size() * sizeof(float), containerPts.data(), GL_DYNAMIC_DRAW); 
            shader.SetVec3("u_Color", glm::vec3(0.5f, 0.5f, 0.5f));
            glDrawArrays(GL_LINE_STRIP, 0, segments + 1); // Bottom
            glDrawArrays(GL_LINE_STRIP, segments + 1, segments + 1); // Top
            glDrawArrays(GL_LINES, (segments + 1) * 2, 4); // Lid Cross
            
            // Restore buffer size for agents next frame
             glBufferData(GL_ARRAY_BUFFER, 1000 * 3 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
            
        } else {
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT); 
        }

        // Render UI on top
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(m_Window);
        glfwPollEvents();
    }
    
    glDeleteVertexArrays(1, &m_AgentVAO);
    glDeleteBuffers(1, &m_AgentVBO);
    glDeleteBuffers(1, &m_ColorVBO);
    glDeleteVertexArrays(1, &m_BondVAO);
    glDeleteBuffers(1, &m_BondVBO);
    glDeleteBuffers(1, &m_BondColorVBO);
}

void Application::Run() {
    MainLoop();
}

void Application::ProcessInput() {
    if (glfwGetKey(m_Window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(m_Window, true);
}

void Application::RenderUI() {
    ImGui::Begin("Control Panel");
    
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::Checkbox("Render Simulation (3D)", &m_RenderSimulation);
    ImGui::Checkbox("Pause", &m_IsPaused);
    ImGui::SliderFloat("Time Scale", &m_TimeScale, 0.0f, 5.0f);
    
    if (ImGui::CollapsingHeader("Realism Parameters", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SliderFloat("Temperature (C)", &m_SimEngine.m_Temperature, 0.0f, 50.0f);
        ImGui::SliderFloat("Rising Rate", &m_SimEngine.m_SpringExpansionRate, 0.0f, 1.0f);
        ImGui::SliderFloat("Bond Probability", &m_SimEngine.m_BondProbability, 0.0f, 1.0f);
        ImGui::Separator();
        ImGui::Text("Volume / Density");
        ImGui::SliderFloat("Collision Radius", &m_SimEngine.m_CollisionRadius, 0.01f, 0.2f);
        ImGui::SliderFloat("Repulsion Stiffness", &m_SimEngine.m_RepulsionK, 1000.0f, 20000.0f);
        ImGui::SliderFloat("Static Friction", &m_SimEngine.m_StaticFriction, 0.0f, 2.0f);
        ImGui::SliderFloat("Dynamic Friction", &m_SimEngine.m_DynamicFriction, 0.0f, 2.0f);
    }
    
    if (ImGui::CollapsingHeader("Environment", ImGuiTreeNodeFlags_DefaultOpen)) {
        const char* items[] = { "Zero G", "Gravity", "Central Force" };
        int currentItem = (int)m_SimEngine.m_GravityMode;
        if (ImGui::Combo("Gravity Mode", &currentItem, items, IM_ARRAYSIZE(items))) {
            m_SimEngine.m_GravityMode = (SimulationEngine::GravityMode)currentItem;
        }
        
        if (m_SimEngine.m_GravityMode == SimulationEngine::CENTRAL) {
            ImGui::SliderFloat("Central Force (K)", &m_SimEngine.m_CentralForceK, 0.0f, 20.0f);
        }
        ImGui::SliderFloat("Mixer Speed", &m_SimEngine.m_Mixer.speed, 0.0f, 5.0f);
    }

    if (ImGui::CollapsingHeader("Analytics", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImPlot::BeginPlot("Network Stats", ImVec2(-1, 300))) { // Increased height
            ImPlot::SetupAxes("Time (s)", "Count", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
            ImPlot::PlotLine("Total Bonds", m_PlotTime.data(), m_PlotBonds.data(), m_PlotTime.size());
            ImPlot::PlotLine("Broken Bonds", m_PlotTime.data(), m_PlotBroken.data(), m_PlotTime.size());
            ImPlot::EndPlot();
        }
        
        if (ImPlot::BeginPlot("Rheology", ImVec2(-1, 300))) { // Increased height
             ImPlot::SetupAxes("Time (s)", "Young's Modulus (Pa)", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
            ImPlot::PlotLine("Young's Modulus", m_PlotTime.data(), m_PlotYoungs.data(), m_PlotTime.size());
            ImPlot::EndPlot();
        }
    }

    ImGui::End();
}
void Application::Cleanup() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    
    glfwDestroyWindow(m_Window);
    glfwTerminate();
}