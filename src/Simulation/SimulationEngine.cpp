#define GLM_ENABLE_EXPERIMENTAL
#include "SimulationEngine.h"
#include <glm/gtc/constants.hpp>
#include <glm/gtx/norm.hpp>
#include <random>
#include <algorithm>
#include <iostream>

void SimulationEngine::Init(int agentCount) {
    m_Agents.clear();
    m_Springs.clear();
    m_Agents.reserve(agentCount);

    std::mt19937 gen(42);
    std::uniform_real_distribution<float> distR(0.0f, m_ContainerRadius * 0.9f); // Keep slightly away from walls
    std::uniform_real_distribution<float> distTheta(0.0f, 2.0f * 3.14159f);
    std::uniform_real_distribution<float> distY(m_FloorY + 0.1f, m_FloorY + 1.0f); // Fill bottom 1.0m
    std::uniform_real_distribution<float> distType(0.0f, 1.0f);

    for (int i = 0; i < agentCount; ++i) {
        float r = std::sqrt(distR(gen)); // Sqrt for uniform area distribution
        float theta = distTheta(gen);
        float y = distY(gen);
        
        glm::vec3 pos(r * cos(theta), y, r * sin(theta));
        
        AgentType type = STARCH;
        float t = distType(gen);
        if (t < 0.30f) type = GLUTENIN;
        else if (t < 0.60f) type = GLIADIN;
        // No YEAST agents anymore
        
        m_Agents.emplace_back(i, pos, type);
    }
}

void SimulationEngine::Update(float dt) {
    // 1. Biology: Yeast Effect (Spring Expansion)
    // Instead of growing particles, we expand the network from within
    for (auto& spring : m_Springs) {
        // Expansion
        if (spring.restLength < m_MaxSpringLength) {
            spring.restLength += m_SpringExpansionRate * dt * 0.01f;
        }
        
        // Compression Limit (Min Length)
        // If spring is too short, we can increase restLength temporarily or just rely on repulsion?
        // Actually, the user asked for "min bond length". This usually means the spring resists compression HARD.
        // We handle this by ensuring the spring force pushes back strongly if length < min.
        // But standard spring physics already does this if restLength > currentLength.
        // So we just ensure restLength never drops below min? No, restLength is the target.
        // We want to ensure the *actual* length doesn't drop.
        // Let's just ensure restLength is at least m_MinSpringLength.
        if (spring.restLength < m_MinSpringLength) {
            spring.restLength = m_MinSpringLength;
        }
    }

    // 2. Clear Forces & Apply Gravity/Central Force
    glm::vec3 center(0.0f, 0.0f, 0.0f);
    
    // Brownian Motion Generator
    static std::mt19937 gen(1337);
    std::uniform_real_distribution<float> distBrown(-1.0f, 1.0f);
    float brownianStrength = m_Temperature * 0.5f; // Scale factor

    #pragma omp parallel for
    for (int i = 0; i < m_Agents.size(); ++i) {
        Agent& a = m_Agents[i];
        a.force = glm::vec3(0.0f);
        
        // Gravity Modes
        if (m_GravityMode == GRAVITY) {
            a.force += m_Gravity * a.mass;
        } else if (m_GravityMode == CENTRAL) {
            glm::vec3 dir = center - a.position;
            a.force += dir * m_CentralForceK;
        }
        // NONE: No external force (floating)
        
        // Brownian Motion (Random Jitter)
        if (m_Temperature > 0.0f) {
            glm::vec3 jitter(distBrown(gen), distBrown(gen), distBrown(gen));
            a.force += jitter * brownianStrength;
        }
    }

    // 2. Spatial Grid Update
    m_Grid.Clear();
    for (auto& agent : m_Agents) {
        m_Grid.AddAgent(&agent);
    }

    // 3. Chemistry: Dynamic Bond Creation
    // Note: Cannot easily parallelize due to m_Springs modification
    for (auto& agent : m_Agents) {
        if (agent.type == STARCH) continue; // Only Glutenin/Gliadin form bonds
        if (agent.connectedAgentIDs.size() >= agent.maxBonds) continue;

            m_Grid.ForEachNeighbor(agent.position, [&](Agent* neighbor) {
                if (agent.id == neighbor->id) return;
                
                float distSq = glm::distance2(agent.position, neighbor->position);
                float collisionRadiusSq = m_CollisionRadius * m_CollisionRadius;
                
                // --- 1. Volume Preservation & Friction ---
                // We check if agents are too close (inside Collision Radius).
                // If so, we apply a Repulsion Force to simulate volume (preventing them from merging).
                if (distSq < collisionRadiusSq && distSq > 0.000001f) {
                    float dist = std::sqrt(distSq);
                    glm::vec3 dir = (agent.position - neighbor->position) / dist;
                    float overlap = m_CollisionRadius - dist;
                    
                    // Repulsion: Proportional to overlap depth (Hooke's Law-ish)
                    glm::vec3 repulsion = dir * overlap * m_RepulsionK;
                    
                    // Friction (Coulomb Model):
                    // Resists relative motion between particles.
                    // F_friction <= mu * F_normal (where F_normal is our Repulsion)
                    glm::vec3 relVel = agent.velocity - neighbor->velocity;
                    float v_normal = glm::dot(relVel, dir);
                    glm::vec3 v_tangent = relVel - v_normal * dir;
                    float vt_len = glm::length(v_tangent);
                    
                    glm::vec3 friction(0.0f);
                    if (vt_len > 0.0001f) {
                        float fn = glm::length(repulsion);
                        // Use Static or Dynamic friction coefficient based on speed
                        float mu = (vt_len < 0.1f) ? m_StaticFriction : m_DynamicFriction;
                        glm::vec3 f_dir = -v_tangent / vt_len;
                        friction = f_dir * fn * mu;
                    }
                    
                    // OPTIMIZATION: Lock-Free Update
                    // We only update the current 'agent'. The 'neighbor' will be updated
                    // when the outer loop reaches it. This avoids race conditions without
                    // using slow #pragma omp critical sections.
                    agent.force += repulsion + friction;
                }

            // 2. Dynamic Bond Creation (Probabilistic)
            // Only Glutenin-Gliadin or Glutenin-Glutenin form bonds
            bool canBond = (agent.type == GLUTENIN && neighbor->type == GLIADIN) || 
                           (agent.type == GLUTENIN && neighbor->type == GLUTENIN);
                           

            // RECALCULATE dist for bonding check (since we only calculated distSq above if close)
            float dist = glm::distance(agent.position, neighbor->position);

            if (canBond && dist < m_BondDistance) {
                // Check probability (simulating time/temperature factor)
                // Higher temp could actually BREAK bonds, but for formation we assume mixing helps. POPRAWIC
                // Let's keep it simple: random chance if close.
                static std::uniform_real_distribution<float> distProb(0.0f, 1.0f);
                if (distProb(gen) < m_BondProbability) {
                    // Check if already connected
                    bool alreadyConnected = false;
                    for (int id : agent.connectedAgentIDs) {
                        if (id == neighbor->id) { alreadyConnected = true; break; }
                    }
                    
                    if (!alreadyConnected && 
                        agent.connectedAgentIDs.size() < agent.maxBonds && 
                        neighbor->connectedAgentIDs.size() < neighbor->maxBonds) {
                        
                        Spring newSpring;
                        newSpring.a = &agent;
                        newSpring.b = neighbor;
                        newSpring.restLength = dist;
                        newSpring.springConstant = m_SpringK;
                        newSpring.breakingThreshold = m_BreakingThreshold;
                        
                        // Critical section for vector modification
                        #pragma omp critical
                        {
                            m_Springs.push_back(newSpring);
                            agent.connectedAgentIDs.push_back(neighbor->id);
                            neighbor->connectedAgentIDs.push_back(agent.id);
                        }
                    }
                }
            }
        });
    }

    // 4. Physics: Accumulate Forces
    m_Time += dt;
    m_Mixer.Update(m_Time);

    #pragma omp parallel for
    for (int i = 0; i < m_Agents.size(); ++i) {
        auto& agent = m_Agents[i];
        
        // Environment Forces (Gravity vs Central)
        // Gravity Modes handled above
        // if (m_UseCentralForce) { ... } removed
        
        // Mixer Collision (Infinite Cylinder)
        float dx = agent.position.x - m_Mixer.position.x;
        float dz = agent.position.z - m_Mixer.position.z;
        float distSq = dx*dx + dz*dz;
        float minDist = m_Mixer.radius + agent.radius;
        
        if (distSq < minDist * minDist) {
            float dist = std::sqrt(distSq);
            glm::vec3 dir(dx/dist, 0.0f, dz/dist);
            float overlap = minDist - dist;
            
            // Push out
            glm::vec3 repulsion = dir * (m_RepulsionK * overlap);
            agent.force += repulsion;
            
            // Friction/Drag from mixer movement could be added here
        }
        
        // Repulsion (Variable Radius)
        m_Grid.ForEachNeighbor(agent.position, [&](Agent* neighbor) {
            if (agent.id == neighbor->id) return;
            
            glm::vec3 dir = agent.position - neighbor->position;
            float dist = glm::length(dir);
            float minDist = agent.radius + neighbor->radius;
            
            if (dist < minDist && dist > 0.0001f) {
                float overlap = minDist - dist;
                glm::vec3 direction = dir / dist;
                glm::vec3 repulsionForce = direction * (m_RepulsionK * overlap);
                
                agent.force += repulsionForce;
            }
        });
    }

    // Spring Forces
    for (auto it = m_Springs.begin(); it != m_Springs.end(); ) {
        Agent* a = it->a;
        Agent* b = it->b;
        
        glm::vec3 dir = b->position - a->position;
        float currentLength = glm::length(dir);
        
        // Stress / Breakage
        if (currentLength > it->breakingThreshold) {
            // Remove bond info from agents
            auto& aCon = a->connectedAgentIDs;
            auto& bCon = b->connectedAgentIDs;
            aCon.erase(std::remove(aCon.begin(), aCon.end(), b->id), aCon.end());
            bCon.erase(std::remove(bCon.begin(), bCon.end(), a->id), bCon.end());
            
            it = m_Springs.erase(it);
            m_BrokenBondsTotal++;
            continue;
        }

        if (currentLength > 0.0001f) {
            glm::vec3 direction = dir / currentLength;
            float displacement = currentLength - it->restLength;
            glm::vec3 force = direction * (it->springConstant * displacement);
            
            a->force += force;
            b->force -= force;
        }
        ++it;
    }

    // 5. Verlet Integration
    for (auto& agent : m_Agents) {
        if (agent.isFixed) continue;

        glm::vec3 tempPos = agent.position;
        glm::vec3 acceleration = agent.force / agent.mass;
        
        // Verlet: pos = pos + (pos - prevPos) * damping + a * dt^2
        glm::vec3 velocity = agent.position - agent.prevPosition;
        agent.position = agent.position + velocity * m_Damping + acceleration * (dt * dt);
        agent.prevPosition = tempPos;

        // Floor Collision
        if (agent.position.y < m_FloorY + agent.radius) {
            float displacementY = agent.position.y - agent.prevPosition.y;
            agent.position.y = m_FloorY + agent.radius;
            
            // Bounce
            agent.prevPosition.y = agent.position.y + displacementY * 0.5f;
            
            // Friction on X/Z
            float oldPrevX = agent.prevPosition.x;
            float oldPrevZ = agent.prevPosition.z;
            agent.prevPosition.x = agent.position.x - (agent.position.x - oldPrevX) * 0.9f;
            agent.prevPosition.z = agent.position.z - (agent.position.z - oldPrevZ) * 0.9f;
        }

        // --- Lid Collision (Hard Clamp) ---
        // Prevents tunneling by strictly clamping the Y position.
        // If an agent tries to go above the lid, we force it down and invert its velocity.
        if (agent.position.y > m_ContainerHeight - agent.radius) {
            float displacementY = agent.position.y - agent.prevPosition.y;
            agent.position.y = m_ContainerHeight - agent.radius; // Hard constraint
            
            // Bounce: Invert the vertical velocity component
            agent.prevPosition.y = agent.position.y + displacementY * 0.5f;
            
            // Apply friction to horizontal movement when hitting the lid
            float oldPrevX = agent.prevPosition.x;
            float oldPrevZ = agent.prevPosition.z;
            agent.prevPosition.x = agent.position.x - (agent.position.x - oldPrevX) * 0.9f;
            agent.prevPosition.z = agent.position.z - (agent.position.z - oldPrevZ) * 0.9f;
        }

        // Cylindrical Container Collision
        float distSq = agent.position.x * agent.position.x + agent.position.z * agent.position.z;
        float maxDist = m_ContainerRadius - agent.radius;
        if (distSq > maxDist * maxDist) {
            float dist = std::sqrt(distSq);
            glm::vec3 dir = glm::vec3(agent.position.x, 0.0f, agent.position.z) / dist;
            
            // Project back to edge
            agent.position.x = dir.x * maxDist;
            agent.position.z = dir.z * maxDist;
            
            // Apply friction to velocity (prevent sliding too fast)
            // We want to kill the outward velocity component
            glm::vec3 velocity = agent.position - agent.prevPosition;
            glm::vec3 tangent = glm::cross(glm::vec3(0,1,0), dir);
            float tangentSpeed = glm::dot(velocity, tangent);
            
            // Simple friction: just dampen previous position towards current
             agent.prevPosition.x = agent.position.x - (agent.position.x - agent.prevPosition.x) * 0.5f;
             agent.prevPosition.z = agent.position.z - (agent.position.z - agent.prevPosition.z) * 0.5f;
        }
    }
    
    // Debug Info (every ~60 frames assuming 0.01s step)
    static int frameCounter = 0;
    frameCounter++;
    if (frameCounter % 100 == 0) {
        // float maxY = -100.0f;
        // for (const auto& agent : m_Agents) {
        //     if (agent.position.y > maxY) maxY = agent.position.y;
        // }
        // std::cout << "Agents: " << m_Agents.size() << " | Bonds: " << m_Springs.size() << " | Max Y: " << maxY << std::endl;
    }
}

float SimulationEngine::GetYoungsModulus() const {
    if (m_Springs.empty()) return 0.0f;
    
    float totalStress = 0.0f;
    for (const auto& spring : m_Springs) {
        float currentLen = glm::distance(spring.a->position, spring.b->position);
        float displacement = std::abs(currentLen - spring.restLength);
        // Stress ~ Force = k * x
        totalStress += spring.springConstant * displacement;
    }
    
    // Average stress per bond
    return totalStress / m_Springs.size();
}