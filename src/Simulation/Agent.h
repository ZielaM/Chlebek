#pragma once
#include <glm/glm.hpp>

enum AgentType {
    GLUTENIN, // Budowniczy (Czerwony)
    GLIADIN,  // Łącznik (Żółty)
    STARCH    // Wypełniacz (Biały)
};

struct Agent {
    // Fizyka
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 force; // Akumulator sił
    
    float mass;
    float radius;
    bool isFixed; // Czy jest zamrożony (do testów)

    // Chemia / Typ
    AgentType type;

    Agent(glm::vec3 pos, AgentType t) 
        : position(pos), type(t), velocity(0.0f), force(0.0f), isFixed(false)
    {
        // Wstępne ustawienia masy w zależności od typu
        if (type == STARCH) { mass = 10.0f; radius = 0.05f; }
        else if (type == GLUTENIN) { mass = 2.0f; radius = 0.03f; }
        else { mass = 1.0f; radius = 0.02f; } // GLIADIN
    }
};