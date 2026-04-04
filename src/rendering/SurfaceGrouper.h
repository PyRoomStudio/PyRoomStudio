#pragma once

#include "core/Types.h"
#include "MeshData.h"

#include <map>
#include <set>
#include <vector>

namespace prs {

class SurfaceGrouper {
  public:
    using EdgeSet = std::set<Edge>;

    static EdgeSet computeFeatureEdges(const MeshData& mesh, float angleThresholdDeg = 10.0f);

    static std::vector<std::set<int>> groupTrianglesIntoSurfaces(const MeshData& mesh, const EdgeSet& featureEdges);
};

} // namespace prs
