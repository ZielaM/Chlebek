#pragma once
#include "Agent.h"

struct Spring {
    Agent* a;
    Agent* b;
    float restLength;
    float springConstant;
    float breakingThreshold;
    
    Spring() = default;
    Spring(Agent* _a, Agent* _b, float _k, float _break)
        : a(_a), b(_b), restLength(glm::distance(_a->position, _b->position)), springConstant(_k), breakingThreshold(_break) {}
};
