#pragma once 

#include "hats/types.hpp"
#include "hats/graph.hpp" 
#include <vector>
#include <utility>
#include <array> 

namespace hats { 

inline constexpr std::size_t kMaxSupportedPlayers = 20;
inline constexpr std::size_t kMaxWorldCount = std::size_t{1} << kMaxSupportedPlayers;
inline constexpr std::size_t kWordBits = 64;
inline constexpr std::size_t kMaxWorldWords =
    (kMaxWorldCount + kWordBits - 1) / kWordBits;

using WorldIndex = std::uint32_t;

struct WorldSet {
    std::array<std::uint64_t, kMaxWorldWords> words{};

    void clear_all() noexcept;
    void set(WorldIndex world_idx) noexcept;
    bool test(WorldIndex world_idx) const noexcept;
    void intersect_with(const WorldSet& other, std::size_t active_word_count) noexcept;
    std::uint32_t count(std::size_t active_word_count) const noexcept;
    bool empty(std::size_t active_word_count) const noexcept;
};

struct KnowledgeState { 
    std::size_t n{0};
    std::size_t world_count{0};
    std::size_t active_word_count{0};
    std::vector<WorldSet> worlds;
};
[[nodiscard]] KnowledgeState init_knowledge(const Graph& g, WorldMask actual_world); 

[[nodiscard]] std::pair<bool, int> can_guess(const KnowledgeState& ks, PlayerId v); 

} // namespace hats