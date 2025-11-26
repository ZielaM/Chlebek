#pragma once
#include "Agent.h"
#include <vector>
#include <cmath>
#include <algorithm>

class SpatialGrid {
public:
    SpatialGrid(float cellSize, int width, int height, int depth)
        : m_CellSize(cellSize), m_Width(width), m_Height(height), m_Depth(depth) 
    {
        m_Grid.resize(width * height * depth);
    }

    void Clear() {
        for (auto& cell : m_Grid) {
            cell.clear();
        }
    }

    void AddAgent(Agent* agent) {
        int index = GetCellIndex(agent->position);
        if (index >= 0 && index < m_Grid.size()) {
            m_Grid[index].push_back(agent);
        }
    }

    // Returns potential neighbors (including self's cell and adjacent cells)
    void GetNeighbors(const glm::vec3& pos, std::vector<Agent*>& outNeighbors) {
        int cx = (int)((pos.x + 5.0f) / m_CellSize); // Offset to handle negative coords
        int cy = (int)((pos.y + 5.0f) / m_CellSize);
        int cz = (int)((pos.z + 5.0f) / m_CellSize);

        for (int z = cz - 1; z <= cz + 1; ++z) {
            for (int y = cy - 1; y <= cy + 1; ++y) {
                for (int x = cx - 1; x <= cx + 1; ++x) {
                    int index = GetIndexFromCoords(x, y, z);
                    if (index >= 0 && index < m_Grid.size()) {
                        outNeighbors.insert(outNeighbors.end(), m_Grid[index].begin(), m_Grid[index].end());
                    }
                }
            }
        }
    }

    template<typename Func>
    void ForEachNeighbor(const glm::vec3& pos, Func func) {
        int cx = (int)((pos.x + 5.0f) / m_CellSize);
        int cy = (int)((pos.y + 5.0f) / m_CellSize);
        int cz = (int)((pos.z + 5.0f) / m_CellSize);

        for (int z = cz - 1; z <= cz + 1; ++z) {
            for (int y = cy - 1; y <= cy + 1; ++y) {
                for (int x = cx - 1; x <= cx + 1; ++x) {
                    int index = GetIndexFromCoords(x, y, z);
                    if (index >= 0 && index < m_Grid.size()) {
                        for (auto* agent : m_Grid[index]) {
                            func(agent);
                        }
                    }
                }
            }
        }
    }

private:
    float m_CellSize;
    int m_Width, m_Height, m_Depth;
    std::vector<std::vector<Agent*>> m_Grid;

    int GetCellIndex(const glm::vec3& pos) {
        int x = (int)((pos.x + 5.0f) / m_CellSize);
        int y = (int)((pos.y + 5.0f) / m_CellSize);
        int z = (int)((pos.z + 5.0f) / m_CellSize);
        return GetIndexFromCoords(x, y, z);
    }

    int GetIndexFromCoords(int x, int y, int z) {
        if (x < 0 || x >= m_Width || y < 0 || y >= m_Height || z < 0 || z >= m_Depth) return -1;
        return x + y * m_Width + z * m_Width * m_Height;
    }
};
