#pragma once

#include "hats/graph.hpp"
#include "hats/types.hpp"
#include <unordered_map>
#include <utility>
#include <vector>

namespace hats {

inline constexpr std::size_t kMaxSupportedPlayers = 20;
inline constexpr std::size_t kWordBits = 64;

using WorldIndex = std::uint32_t;

// Opt 3: dynamic size — vector sized to active_word_count, not 128 KB fixed array.
struct WorldSet {
    std::vector<std::uint64_t> words;

    WorldSet() = default;
    explicit WorldSet(std::size_t word_count) : words(word_count, 0) {}

    void clear_all() noexcept;
    void set(WorldIndex world_idx) noexcept;
    bool test(WorldIndex world_idx) const noexcept;
    // Opt 1: active_word_count dropped — vector knows its own size.
    void intersect_with(const WorldSet &other) noexcept;
    void subtract(const WorldSet &other) noexcept;
    void union_with(const WorldSet &other) noexcept;
    std::uint32_t count() const noexcept;
    bool empty() const noexcept;
};

struct KnowledgeState {
    std::size_t n{0};
    std::size_t world_count{0};
    std::size_t active_word_count{0};
    // Opt 2: actual_views replaces worlds — computed once, O(n) storage.
    std::vector<VertexMask> actual_views;
};
[[nodiscard]] KnowledgeState init_knowledge(const Graph &g, WorldMask actual_world);

// Opt 2: takes partition class + global valid set instead of ks_.worlds[v].
[[nodiscard]] std::pair<bool, int> can_guess(const std::vector<WorldIndex> &cls,
                                             const WorldSet &global_valid_worlds, PlayerId v);

// Maps each possible view (neighbors' bits) to the list of worlds consistent with that view.
struct AgentPartition {
    VertexMask neighbors_mask{0};
    std::unordered_map<VertexMask, std::vector<WorldIndex>> classes;
};

using Partitions = std::vector<AgentPartition>;

// Precomputes AgentPartition for every agent. Call once; reuse across all silence rounds.
[[nodiscard]] Partitions compute_partitions(const Graph &g, std::size_t world_count);

} // namespace hats