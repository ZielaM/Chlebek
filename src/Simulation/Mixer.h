#pragma once
#include <glm/glm.hpp>
#include <cmath>

struct Mixer {
    glm::vec3 position;
    float radius;
    float speed;
    
    // Lissajous parameters
    float A = 0.5f;
    float B = 0.5f;
    float a = 3.0f;
    float b = 2.0f;

    Mixer() : position(0.0f), radius(0.1f), speed(1.0f) {}

    void Update(float time) {
        float t = time * speed;
        position.x = A * std::sin(a * t);
        position.z = B * std::cos(b * t);
        // Y is irrelevant for an infinite vertical rod
    }
};
