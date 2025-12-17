#define GLM_ENABLE_EXPERIMENTAL
#include "SimulationEngine.h"
#include <glm/gtc/constants.hpp>
#include <glm/gtx/norm.hpp>
#include <random>
#include <algorithm>
#include <iostream>

void SimulationEngine::Init(int gliadinCount, int gluteninCount, int starchCount) {
    int totalAgents = gliadinCount + gluteninCount + starchCount;
    m_Agents.clear();
    m_Springs.clear();
    m_BrokenBondsTotal = 0;
    m_Agents.reserve(totalAgents);

    std::mt19937 gen(42);
    std::uniform_real_distribution<float> distR(0.0f, m_ContainerRadius * 0.9f); // Keep slightly away from walls
    std::uniform_real_distribution<float> distTheta(0.0f, 2.0f * 3.14159f);
    std::uniform_real_distribution<float> distY(m_FloorY + 0.1f, m_FloorY + 1.0f); // Fill bottom 1.0m

    // Create list of types to assign
    std::vector<AgentType> types;
    types.reserve(totalAgents);
    for (int i = 0; i < gliadinCount; ++i) types.push_back(GLIADIN);
    for (int i = 0; i < gluteninCount; ++i) types.push_back(GLUTENIN);
    for (int i = 0; i < starchCount; ++i) types.push_back(STARCH);

    // Shuffle types to randomize distribution
    std::shuffle(types.begin(), types.end(), gen);

    for (int i = 0; i < totalAgents; ++i) {
        float r = std::sqrt(distR(gen)); // Sqrt for uniform area distribution
        float theta = distTheta(gen);
        float y = distY(gen);
        
        glm::vec3 pos(r * cos(theta), y, r * sin(theta));
        
        m_Agents.emplace_back(i, pos, types[i]);
    }

    // --- Settling Phase: Gently separate overlapping agents ---
    float settlingDt = 0.016f;
    for (int step = 0; step < 60; ++step) {
        // 1. Rebuild Grid
        m_Grid.Clear();
        for (auto& agent : m_Agents) {
            m_Grid.AddAgent(&agent);
        }

        // 2. Calculate Repulsion
        #pragma omp parallel for
        for (int i = 0; i < m_Agents.size(); ++i) {
            Agent& a = m_Agents[i];
            a.force = glm::vec3(0.0f); // Zero out gravity/thermal

            m_Grid.ForEachNeighbor(a.position, [&](Agent* neighbor) {
                if (a.id == neighbor->id) return;

                float distSq = glm::distance2(a.position, neighbor->position);
                float radiusSum = m_CollisionRadius; // Push to equilibrium distance

                if (distSq < radiusSum * radiusSum && distSq > 0.000001f) {
                    float dist = std::sqrt(distSq);
                    glm::vec3 dir = (a.position - neighbor->position) / dist;
                    float overlap = radiusSum - dist;
                    a.force += dir * overlap * m_RepulsionK;
                }
            });
        }

        // 3. Integrate & Constrain
        #pragma omp parallel for
        for (int i = 0; i < m_Agents.size(); ++i) {
            Agent& a = m_Agents[i];
            glm::vec3 acc = a.force / a.mass;
            glm::vec3 tempPos = a.position;

            // High damping (0.1f) to kill kinetic energy
            a.position = a.position + (a.position - a.prevPosition) * 0.1f + acc * (settlingDt * settlingDt);
            a.prevPosition = tempPos;

            // Constraints
            if (a.position.y < m_FloorY + a.radius) a.position.y = m_FloorY + a.radius;
            if (a.position.y > m_ContainerHeight - a.radius) a.position.y = m_ContainerHeight - a.radius;
            
            float d2 = a.position.x * a.position.x + a.position.z * a.position.z;
            float maxR = m_ContainerRadius - a.radius;
            if (d2 > maxR * maxR) {
                float d = std::sqrt(d2);
                a.position.x = (a.position.x / d) * maxR;
                a.position.z = (a.position.z / d) * maxR;
            }
        }
    }
}

void SimulationEngine::Update(float dt) {
    // 1. Biology: Yeast Effect (Spring Expansion)
    for (auto& spring : m_Springs) {
        // Expansion
        if (spring.restLength < m_MaxSpringLength) {
            spring.restLength += m_SpringExpansionRate * dt * 0.05f;
        }
        
        // Compression Limit (Min Length)
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
    for (auto& agent : m_Agents) {
        if (agent.type == STARCH) continue; // Only Glutenin/Gliadin form bonds
        if (agent.connectedAgentIDs.size() >= agent.maxBonds) continue;

        m_Grid.ForEachNeighbor(agent.position, [&](Agent* neighbor) {
            if (agent.id == neighbor->id) return;

            float distSq = glm::distance2(agent.position, neighbor->position);
            float collisionRadiusSq = m_CollisionRadius * m_CollisionRadius;

            // --- 1. Volume Preservation & Friction ---
            if (distSq < collisionRadiusSq && distSq > 0.000001f) {
                float dist = std::sqrt(distSq);
                glm::vec3 dir = (agent.position - neighbor->position) / dist;
                float overlap = m_CollisionRadius - dist;

                // Repulsion: Proportional to overlap depth (Hooke's Law-ish)
                glm::vec3 repulsion = dir * overlap * m_RepulsionK;

                // Friction (Coulomb Model):
                glm::vec3 relVel = agent.velocity - neighbor->velocity;
                float v_normal = glm::dot(relVel, dir);
                glm::vec3 v_tangent = relVel - v_normal * dir;
                float vt_len = glm::length(v_tangent);

                glm::vec3 friction(0.0f);
                if (vt_len > 0.0001f) {
                    float fn = glm::length(repulsion);
                    float mu = (vt_len < 0.1f) ? m_StaticFriction : m_DynamicFriction;
                    glm::vec3 f_dir = -v_tangent / vt_len;
                    friction = f_dir * fn * mu;
                }

                agent.force += repulsion + friction;
            }

            // 2. Dynamic Bond Creation (Probabilistic)
            bool canBond = (agent.type == GLUTENIN && neighbor->type == GLIADIN) ||
                (agent.type == GLUTENIN && neighbor->type == GLUTENIN);


            float dist = glm::distance(agent.position, neighbor->position);

            if (canBond && dist < m_BondDistance) {
                static std::uniform_real_distribution<float> distProb(0.0f, 1.0f);
                if (distProb(gen) < m_BondProbability) {
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

    //3.5. CHEMISTRY: DYNAMIC STARCH ANCHORING (Skrobia wchodzi w sklad Ciasta)

            const float STARCH_BOND_DISTANCE = 4.0f;
            const float STARCH_REST_LENGTH = 0.1f;
            const float STARCH_SPRING_MULTIPLIER = 15.0f;

            static std::uniform_real_distribution<float> distProb(0.0f, 1.0f);
            for (auto& starchAgent : m_Agents) {
                if (starchAgent.type != STARCH || starchAgent.isStarchBonded) continue;

                m_Grid.ForEachNeighbor(starchAgent.position, [&](Agent* neighbor) {
                    if (neighbor->type == STARCH || starchAgent.id == neighbor->id) return;
                    if (neighbor->connectedAgentIDs.empty()) return;

                    float dist = glm::distance(starchAgent.position, neighbor->position);
                    if (dist < STARCH_BOND_DISTANCE) {
                        if (distProb(gen) < m_BondProbability * 2000.0f) {
                            //nowe krotkie sztywne wiazanie
                            Spring newStarchSpring;
                            newStarchSpring.a = &starchAgent;
                            newStarchSpring.b = neighbor;

                            newStarchSpring.restLength = STARCH_REST_LENGTH;
                            newStarchSpring.springConstant = m_SpringK * STARCH_SPRING_MULTIPLIER;
                            newStarchSpring.breakingThreshold = STARCH_REST_LENGTH + 0.5f;

#pragma omp critical

                            {
                                m_Springs.push_back(newStarchSpring);
                                starchAgent.isStarchBonded = true;
                            }

                            return;

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
        
        // Mixer Collision 
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
            
            // Drag: Pull agent along with mixer
            glm::vec3 relVel = agent.velocity - m_Mixer.velocity;
            
            float mixerDragCoeff = 10.0f; 
            glm::vec3 dragForce = -relVel * mixerDragCoeff;
            
            agent.force += dragForce;
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
        
        // Verlet
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
        if (agent.position.y > m_ContainerHeight - agent.radius) {
            float displacementY = agent.position.y - agent.prevPosition.y;
            agent.position.y = m_ContainerHeight - agent.radius;
            
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

            glm::vec3 velocity = agent.position - agent.prevPosition;
            glm::vec3 tangent = glm::cross(glm::vec3(0,1,0), dir);
            float tangentSpeed = glm::dot(velocity, tangent);
            
            // Simple friction
             agent.prevPosition.x = agent.position.x - (agent.position.x - agent.prevPosition.x) * 0.5f;
             agent.prevPosition.z = agent.position.z - (agent.position.z - agent.prevPosition.z) * 0.5f;
        }
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