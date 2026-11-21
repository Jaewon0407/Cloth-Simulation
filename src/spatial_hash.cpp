#include "../header/spatial_hash.h"

void SpatialHash::build(const std::vector<glm::vec3>& points, float cellSize)
{
    invCellSize_ = 1.0f / cellSize;
    tableSize_ = static_cast<int>(points.size()) * 2 + 1;

    cellStart_.assign(tableSize_ + 1, 0);
    entries_.resize(points.size());

    for (const auto& p : points)
    {
        const glm::ivec3 c = cellOf(p);
        cellStart_[hashCell(c.x, c.y, c.z)]++;
    }

    int sum = 0;
    for (int i = 0; i < tableSize_; ++i)
    {
        sum += cellStart_[i];
        cellStart_[i] = sum;
    }
    cellStart_[tableSize_] = sum;

    for (int i = 0; i < static_cast<int>(points.size()); ++i)
    {
        const glm::ivec3 c = cellOf(points[i]);
        const int bucket = hashCell(c.x, c.y, c.z);
        cellStart_[bucket]--;
        entries_[cellStart_[bucket]] = i;
    }
}
