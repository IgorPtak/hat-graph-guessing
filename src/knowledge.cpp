#include "hats/knowledge.hpp"

#include <stdexcept>

namespace hats { 

void WorldSet::clear_all() noexcept { 
    for (std::size_t i{0}; i < kMaxWorldWords; ++i) { 
        words[i] = 0;
    }
}

void WorldSet::set(WorldIndex world_idx) noexcept { 
    const std::size_t word = world_idx / kWordBits; 
    const std::size_t bit  = world_idx % kWordBits; 
    words[word] |= (1ULL << bit); 
}

bool WorldSet::test(WorldIndex world_idx) const noexcept { 
    const std::size_t word = world_idx / kWordBits; 
    const std::size_t bit  = world_idx % kWordBits; 
    return (words[word] & (1ULL << bit)) != 0;
}

std::uint32_t WorldSet::count(size_t active_word_count) const noexcept { 
    std::uint32_t total = 0; 
    for (std::size_t i{0}; i < active_word_count; ++i) { 
        total += static_cast<std::uint32_t>(__builtin_popcountll(words[i])); 
    }
    return total;
}

void WorldSet::intersect_with(const WorldSet& other, std::size_t active_word_count) noexcept {
    for (std::size_t i = 0; i < active_word_count; ++i) { 
        words[i] &= other.words[i]; 
    }
}

bool WorldSet::empty(std::size_t active_word_count) const noexcept { 
    for (std::size_t i = 0; i < active_word_count; ++i) { 
        if (words[i] != 0) return false;
    }
    return true;
}


KnowledgeState init_knowledge(const Graph& g, WorldMask actual_world) { 
    const std::size_t n = g.size();
    if (n == 0 || n > kMaxSupportedPlayers) {
        throw std::invalid_argument("init_knowledge: n must be in [1, 20]");
    }

    KnowledgeState ks{}; 
    ks.n = n; 
    ks.world_count = std::size_t{1} << n; 
    ks.active_word_count = (ks.world_count + kWordBits - 1) / kWordBits;
    ks.worlds.resize(n);

    for (std::size_t i = 0; i < n; ++i) { 
        const PlayerId player = static_cast<PlayerId>(i);
        const VertexMask neighbors_mask = g.neighbors(player);
        const VertexMask view_i = actual_world & neighbors_mask;

        for (std::size_t w = 1; w < ks.world_count; ++w) {
            if ((static_cast<WorldMask>(w) & neighbors_mask) == view_i) {
                ks.worlds[player].set(static_cast<WorldIndex>(w));
            }
        }
    }
    return ks;
}

std::pair<bool, int> can_guess(const KnowledgeState& ks, PlayerId v) { 
    if (static_cast<std::size_t>(v) >= ks.n) {
        throw std::out_of_range("can_guess: player index out of range");
    }
    const WorldSet& candidates = ks.worlds[v];
    if (candidates.empty(ks.active_word_count)) return {false, -1};

    bool seen_any = false;
    int decided_color = -1;

    for (std::size_t w = 1; w < ks.world_count; ++w) {
        const auto wi = static_cast<WorldIndex>(w);
        if (!candidates.test(wi)) continue;

        const int color = test_bit(static_cast<WorldMask>(w), v) ? 1 : 0;

        if (!seen_any) {
            decided_color = color;
            seen_any = true;
        } else if (color != decided_color) {
            return {false, -1};
        }
    }

    return seen_any ? std::pair<bool, int>{true, decided_color}
                    : std::pair<bool, int>{false, -1};
}

} // namespace hats