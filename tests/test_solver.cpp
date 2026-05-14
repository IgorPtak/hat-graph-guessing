#include <cassert>
#include <iostream>

#include "hats/graph.hpp"
#include "hats/knowledge.hpp"
#include "solver/solver.hpp"

void test_solver_complete_graph_1_red() {
    hats::Graph g = hats::Graph::make_complete(3);
    hats::WorldMask actual = 0b001; 

    solver::Solver s(g, actual);
    solver::SimulationResult result = s.run();

    assert(result.success == true);
    assert(result.deadlock == false);
    assert(result.rounds == 1);
    std::cout << "Test Complete Graph (1 red) passed!\n";
}

void test_solver_complete_graph_2_red() {
    hats::Graph g = hats::Graph::make_complete(3);
    hats::WorldMask actual = 0b011; 

    solver::Solver s(g, actual);
    solver::SimulationResult result = s.run();

    assert(result.success == true);
    assert(result.deadlock == false);
    assert(result.rounds == 2);
    std::cout << "Test Complete Graph (2 red) passed!\n";
}

void test_solver_star_graph_deadlock() {
    hats::Graph g = hats::Graph::make_star(3);
    hats::WorldMask actual = 0b110;

    solver::Solver s(g, actual);
    solver::SimulationResult result = s.run();

    assert(result.success == false);
    assert(result.deadlock == true);
    std::cout << "Test Star Graph (deadlock) passed!\n";
}

int main() {
    std::cout << "Running Solver Tests...\n";
    test_solver_complete_graph_1_red();
    test_solver_complete_graph_2_red();
    test_solver_star_graph_deadlock();
    std::cout << "All Solver tests passed successfully!\n";
    return 0;
}