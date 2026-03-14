#pragma once 

#include "hats/types.hpp"
#include "hats/graph.hpp" 
#include <vector>
#include <array> 

namespace hats { 

using WorldSet = std::vector<WorldMask>; 

struct KnowledgeState { 
    std::size_t n; 
    std::array<WorldSet, kMaxPlayers> worlds; 
};
[[nodiscard]] KnowledgeState init_knowledge(const Graph& g, WorldMask actual_world); 

[[nodiscard]] std::pair<bool, int> can_guess(const KnowledgeState& ks, PlayerId v); 

} // namespace hats