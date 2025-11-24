#pragma once
#include "Agent.h"
#include <vector>

class SimulationEngine {
public:
    SimulationEngine();
    
    // Inicjalizuje symulację (tworzy agentów)
    void Init(int count);

    // Wykonuje krok fizyki (dt = delta time)
    void Update(float dt);

    // Dostęp do danych dla Renderera
    const std::vector<Agent>& GetAgents() const { return m_Agents; }

private:
    std::vector<Agent> m_Agents;
    
    // Parametry świata
    glm::vec3 m_Gravity = glm::vec3(0.0f, -9.81f, 0.0f);
    float m_Damping = 0.99f; // Tłumienie (opór powietrza/wody)
    float m_FloorY = -1.0f;  // Poziom podłogi
};