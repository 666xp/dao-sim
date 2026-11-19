#pragma once

#include "graph.h"

namespace dao {

// Roche-inspired static coupling weight in [0,1] for dao <-> field projections.
// Masses of left/right sediments shape a soft potential; neurons near the
// "Lagrange" midplane get stronger symmetric coupling.
float roche_weight(Region src, Region dst, uint32_t src_local, uint32_t dst_local,
                   uint32_t src_count, uint32_t dst_count);

}  // namespace dao
