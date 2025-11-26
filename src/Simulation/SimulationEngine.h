#pragma once
#include "Agent.h"
#include "Spring.h"
#include "SpatialGrid.h"
#include "Mixer.h"
#include <vector>

class SimulationEngine {
public:
    SimulationEngine() : m_Grid(0.2f, 50, 50, 50) {}
    void Init(int count);
    void Update(float dt); 
    const std::vector<Agent>& GetAgents() const { return m_Agents; }

private:
    std::vector<Agent> m_Agents;
    // Parameters
    glm::vec3 m_Gravity = glm::vec3(0.0f, -9.81f, 0.0f);
    float m_FloorY = -1.0f;
    float m_ContainerRadius = 1.0f; // Cylindrical container radius (Widened to 1.0)
    float m_ContainerHeight = 1.5f; // Lid height (lowered for visibility)
    
    float m_Damping = 0.99f; // Air resistance
    
    // Physics & Chemistry
    std::vector<Spring> m_Springs;
    SpatialGrid m_Grid;
    
    // --- Physics Parameters ---
    float m_SpringK = 2500.0f;          // Stiffness of the gluten bonds
    float m_RepulsionK = 1000.0f;       // Repulsion force to maintain volume
    float m_CollisionRadius = 0.1f;     // Radius at which repulsion activates
    float m_StaticFriction = 0.0f;      // Friction when relative velocity is low
    float m_DynamicFriction = 0.0f;     // Friction when sliding
    
    float m_BondDistance = 0.15f;       // Max distance to form a bond
    float m_BreakingThreshold = 0.5f;   // Max length before a bond breaks
    float m_MinSpringLength = 0.05f;    // Minimum length (compression limit)
    
    // --- Biology (Yeast Simulation) ---
    float m_SpringExpansionRate = 0.1f; // Rate at which springs expand (Rising)
    float m_MaxSpringLength = 0.5f;     // Maximum length a spring can grow to
    
    // --- Realism ---
    float m_Temperature = 25.0f;        // Controls Brownian motion intensity
    float m_BondProbability = 0.1f;     // Probability of forming a bond per frame
    
    // --- Environment ---
    Mixer m_Mixer;
    enum GravityMode { NONE, GRAVITY, CENTRAL };
    GravityMode m_GravityMode = NONE;
    float m_CentralForceK = 5.0f;       // Strength of central pull
    float m_Time = 0.0f;
    
    // Analytics
    int m_BrokenBondsTotal = 0;
    float GetYoungsModulus() const;
    
    friend class Application;
};