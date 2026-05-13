#pragma once

#include "hats/types.hpp"
#include "hats/graph.hpp"
#include "hats/knowledge.hpp" 
#include <vector>

namespace solver {

struct SimulationResult {
    bool success;
    bool deadlock;
    int rounds;
};

class Solver {
    public:
        Solver(const hats::Graph &graph, hats::WorldMask actual_world);

        SimulationResult run();

    private:
        bool apply_silence();

        const hats::Graph &graph_;
        hats::KnowledgeState ks_;
        hats::WorldSet global_valid_worlds_;
};

} // namespace solver

