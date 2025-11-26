#pragma once
#include <glm/glm.hpp>
#include <vector>

// Added YEAST
enum AgentType { GLUTENIN, GLIADIN, STARCH, YEAST };

struct Agent {
    int id;
    glm::vec3 position;
    glm::vec3 prevPosition; 
    glm::vec3 velocity;     
    glm::vec3 force;
    float mass;
    float radius;
    bool isFixed;
    AgentType type;
    
    // Chemistry Memory
    int maxBonds;
    std::vector<int> connectedAgentIDs;

    Agent(int id, glm::vec3 pos, AgentType t) 
        : id(id), position(pos), prevPosition(pos), type(t), velocity(0.0f), force(0.0f), isFixed(false)
    {
        if (type == STARCH) { mass = 10.0f; radius = 0.05f; maxBonds = 0; }
        else if (type == GLUTENIN) { mass = 2.0f; radius = 0.03f; maxBonds = 4; }
        else if (type == GLIADIN) { mass = 1.0f; radius = 0.02f; maxBonds = 2; }
        else if (type == YEAST) { mass = 5.0f; radius = 0.04f; maxBonds = 0; } // Yeast start small
    }
};