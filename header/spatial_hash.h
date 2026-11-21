#pragma once
#include <glm/glm.hpp>
#include <cmath>
#include <cstdlib>
#include <vector>

class SpatialHash
{
public:
    void build(const std::vector<glm::vec3>& points, float cellSize);

    template <typename Fn>
    void forEachNearby(const glm::vec3& p, Fn&& fn) const
    {
        const glm::ivec3 c = cellOf(p);
        for (int x = c.x - 1; x <= c.x + 1; ++x)
            for (int y = c.y - 1; y <= c.y + 1; ++y)
                for (int z = c.z - 1; z <= c.z + 1; ++z)
                {
                    const int bucket = hashCell(x, y, z);
                    for (int k = cellStart_[bucket]; k < cellStart_[bucket + 1]; ++k)
                        fn(entries_[k]);
                }
    }

private:
    glm::ivec3 cellOf(const glm::vec3& p) const
    {
        return glm::ivec3(glm::floor(p * invCellSize_));
    }

    int hashCell(int x, int y, int z) const
    {
        const long long h = (static_cast<long long>(x) * 92837111) ^
                            (static_cast<long long>(y) * 689287499) ^
                            (static_cast<long long>(z) * 283923481);
        return static_cast<int>(std::llabs(h) % tableSize_);
    }

    float invCellSize_ = 1.0f;
    int tableSize_ = 1;
    std::vector<int> cellStart_;
    std::vector<int> entries_;
};
