#include "hats/knowledge.hpp"

#include <stdexcept>
#ifdef _OPENMP
#include <omp.h>
#endif

namespace hats {

void WorldSet::clear_all() noexcept {
    for (auto &w : words)
        w = 0;
}

void WorldSet::set(WorldIndex world_idx) noexcept {
    const std::size_t word = world_idx / kWordBits;
    const std::size_t bit = world_idx % kWordBits;
    words[word] |= (1ULL << bit);
}

bool WorldSet::test(WorldIndex world_idx) const noexcept {
    const std::size_t word = world_idx / kWordBits;
    const std::size_t bit = world_idx % kWordBits;
    return (words[word] & (1ULL << bit)) != 0;
}

std::uint32_t WorldSet::count() const noexcept {
    std::uint32_t total = 0;
    for (const auto w : words)
        total += static_cast<std::uint32_t>(__builtin_popcountll(w));
    return total;
}

void WorldSet::intersect_with(const WorldSet &other) noexcept {
    for (std::size_t i = 0; i < words.size(); ++i)
        words[i] &= other.words[i];
}

void WorldSet::subtract(const WorldSet &other) noexcept {
    for (std::size_t i = 0; i < words.size(); ++i)
        words[i] &= ~other.words[i];
}

void WorldSet::union_with(const WorldSet &other) noexcept {
    for (std::size_t i = 0; i < words.size(); ++i)
        words[i] |= other.words[i];
}

bool WorldSet::empty() const noexcept {
    for (const auto w : words)
        if (w != 0) return false;
    return true;
}

KnowledgeState init_knowledge(const Graph &g, WorldMask actual_world) {
    const std::size_t n = g.size();
    if (n == 0 || n > kMaxSupportedPlayers)
        throw std::invalid_argument("init_knowledge: n must be in [1, 20]");

    KnowledgeState ks{};
    ks.n = n;
    ks.world_count = std::size_t{1} << n;
    ks.active_word_count = (ks.world_count + kWordBits - 1) / kWordBits;
    ks.actual_views.resize(n);

    for (std::size_t i = 0; i < n; ++i) {
        const PlayerId player = static_cast<PlayerId>(i);
        const VertexMask neighbors_mask = g.neighbors(player);
        ks.actual_views[player] = actual_world & neighbors_mask;
    }
    return ks;
}

std::pair<bool, int> can_guess(
    const std::vector<WorldIndex> &cls,
    const WorldSet &global_valid_worlds,
    PlayerId v)
{
    bool seen_zero = false;
    bool seen_one = false;

    for (const WorldIndex w : cls) {
        if (!global_valid_worlds.test(w)) continue;
        if (test_bit(static_cast<WorldMask>(w), v))
            seen_one = true;
        else
            seen_zero = true;
        if (seen_zero && seen_one) return {false, -1};
    }

    if (!seen_zero && !seen_one) return {false, -1};
    return {true, seen_one ? 1 : 0};
}

Partitions compute_partitions(const Graph &g, std::size_t world_count) {
    const std::size_t n = g.size();
    Partitions parts(n);

    #pragma omp parallel for schedule(static)
    for (std::size_t i = 0; i < n; i++) {
        parts[i].neighbors_mask = g.neighbors(static_cast<PlayerId>(i));
        parts[i].classes.reserve(world_count / 2);
        for (std::size_t w = 1; w < world_count; w++) {
            const VertexMask view = static_cast<WorldMask>(w) & parts[i].neighbors_mask;
            parts[i].classes[view].push_back(static_cast<WorldIndex>(w));
        }
    }
    return parts;
}

} // namespace hats