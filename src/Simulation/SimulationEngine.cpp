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

        // 1. Zeruj siły i dodaj grawitację
        agent.force = glm::vec3(0.0f);
        agent.force += m_Gravity * agent.mass;

        // 2. Całkowanie (Fizyka Newtona: F = ma -> a = F/m)
        glm::vec3 acceleration = agent.force / agent.mass;
        
        agent.velocity += acceleration * dt;
        agent.velocity *= m_Damping; // Opór ośrodka
        agent.position += agent.velocity * dt;

        // 3. Proste kolizje z podłogą (Miska)
        if (agent.position.y < m_FloorY) {
            agent.position.y = m_FloorY;
            agent.velocity.y *= -0.5f; // Odbicie z utratą energii
        }
    }
}