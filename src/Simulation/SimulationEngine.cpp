#include "SimulationEngine.h"
#include <random>

SimulationEngine::SimulationEngine() {}

void SimulationEngine::Init(int count) {
    m_Agents.clear();
    
    // Generator losowy
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> distX(-0.8f, 0.8f);
    std::uniform_real_distribution<float> distY(0.0f, 2.0f); // Zaczynają wysoko
    std::uniform_real_distribution<float> distZ(-0.8f, 0.8f);
    std::uniform_int_distribution<int> typeDist(0, 2);

    for (int i = 0; i < count; i++) {
        glm::vec3 startPos(distX(rng), distY(rng), distZ(rng));
        AgentType type = (AgentType)typeDist(rng);
        m_Agents.emplace_back(startPos, type);
    }
}

void SimulationEngine::Update(float dt) {
    for (auto& agent : m_Agents) {
        if (agent.isFixed) continue;

        // 1. Siły
        agent.force = glm::vec3(0.0f);
        agent.force += m_Gravity * agent.mass;

        // 2. Całkowanie
        glm::vec3 acceleration = agent.force / agent.mass;
        agent.velocity += acceleration * dt;
        
        // Lepiej najpierw policzyć nową pozycję
        glm::vec3 nextPos = agent.position + agent.velocity * dt;

        // 3. Kolizja z podłogą (Ulepszona)
        if (nextPos.y < m_FloorY + agent.radius) { // Uwzględniamy promień!
            // Wyciągamy go na powierzchnię
            nextPos.y = m_FloorY + agent.radius; 
            
            // Odwracamy prędkość z utratą energii
            agent.velocity.y *= -0.5f; 
            
            // Proste tarcie o podłogę (żeby nie ślizgały się jak na lodzie)
            agent.velocity.x *= 0.9f;
            agent.velocity.z *= 0.9f;
        }

        agent.position = nextPos;
        agent.velocity *= m_Damping; // Opór powietrza
    }
}