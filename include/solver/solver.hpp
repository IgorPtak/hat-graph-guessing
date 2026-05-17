#pragma once

#include "hats/types.hpp"
#include "hats/graph.hpp"
#include "hats/knowledge.hpp"
#include <cstdint>
#include <vector>

namespace solver {

struct SimulationResult {
    bool success;
    bool deadlock;
    int rounds;
};

struct RoundRecord {
    int round;
    std::vector<int> guessed_players;
    std::vector<int> guessed_colors;
    std::vector<std::uint32_t> valid_worlds;
    // |[i, view_i] ∩ W_valid| for each agent i: number of worlds still consistent with agent i's observation
    std::vector<std::uint32_t> agent_class_sizes;
    bool silence;
};

using SimulationTrace = std::vector<RoundRecord>;

class Solver {
    public:
        Solver(const hats::Graph &graph, hats::WorldMask actual_world);

        SimulationResult run();
        SimulationTrace trace();

    private:
        bool apply_silence(const std::vector<bool>& already_guessed);

        const hats::Graph &graph_;
        hats::KnowledgeState ks_;
        hats::Partitions parts_;
        hats::WorldSet global_valid_worlds_;
};

} // namespace solver

