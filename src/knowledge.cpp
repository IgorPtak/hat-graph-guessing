#include "hats/knowledge.hpp"

#include <stdexcept>

namespace hats { 

KnowledgeState init_knowledge(const Graph& g, WorldMask actual_world) { 
    const std::size_t n = g.size();
    if (n > 20) {
        throw std::invalid_argument("init_knowledge currently supports n <= 20");
    }

    KnowledgeState kn{n};
    const WorldMask max_world = WorldMask{1} << n;

    for (std::size_t i = 0; i < n; ++i) {
        const PlayerId player = static_cast<PlayerId>(i);
        const VertexMask neighbors_mask = g.neighbors(player);
        const VertexMask view_i = actual_world & neighbors_mask;

        kn.worlds[player].reserve(static_cast<std::size_t>(max_world / 2));
        for (WorldMask w = 1; w < max_world; ++w) {
            if ((w & neighbors_mask) == view_i) {
                kn.worlds[player].push_back(w);
            }
        }
    }

    return kn;
}

std::pair<bool, int> can_guess(const KnowledgeState& ks, PlayerId v) { 
    if (static_cast<std::size_t>(v) >= ks.n) {
        throw std::out_of_range("player index out of range");
    }

    const WorldSet& candidates = ks.worlds[v];
    if (candidates.empty()) {
        return {false, -1};
    }

    const int first_color = test_bit(candidates.front(), v) ? 1 : 0;
    for (WorldMask w : candidates) {
        const int color = test_bit(w, v) ? 1 : 0;
        if (color != first_color) {
            return {false, -1};
        }
    }

    return {true, first_color};
}

} // namespace hats