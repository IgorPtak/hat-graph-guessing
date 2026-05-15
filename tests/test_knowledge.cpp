#include <cassert>
#include <iostream>
#include <stdexcept>

#include "hats/graph.hpp"
#include "hats/knowledge.hpp"

void test_init_knowledge_complete_graph_n3() {
    hats::Graph g(3);
    g.add_edge(0, 1);
    g.add_edge(0, 2);
    g.add_edge(1, 2);

    const hats::WorldMask actual = 0b001;
    const hats::KnowledgeState ks = hats::init_knowledge(g, actual);
    const hats::Partitions parts = hats::compute_partitions(g, ks.world_count);

    assert(ks.n == 3);

    // Player 0 sees players 1 and 2 as 0,0. Only world 001 matches.
    assert(parts[0].classes.at(ks.actual_views[0]).size() == 1);

    // Player 1 sees (player 0 = 1, player 2 = 0), own bit can be 0 or 1.
    assert(parts[1].classes.at(ks.actual_views[1]).size() == 2);

    // Player 2 sees (player 0 = 1, player 1 = 0), own bit can be 0 or 1.
    assert(parts[2].classes.at(ks.actual_views[2]).size() == 2);
}

void test_can_guess_complete_graph_n3() {
    hats::Graph g(3);
    g.add_edge(0, 1);
    g.add_edge(0, 2);
    g.add_edge(1, 2);

    const hats::WorldMask actual = 0b001;
    const hats::KnowledgeState ks = hats::init_knowledge(g, actual);
    const hats::Partitions parts = hats::compute_partitions(g, ks.world_count);

    hats::WorldSet global_valid(ks.active_word_count);
    for (std::size_t w = 1; w < ks.world_count; ++w)
        global_valid.set(static_cast<hats::WorldIndex>(w));

    const auto [knows0, color0] = hats::can_guess(
        parts[0].classes.at(ks.actual_views[0]), global_valid, 0);
    assert(knows0);
    assert(color0 == 1);

    const auto [knows1, color1] = hats::can_guess(
        parts[1].classes.at(ks.actual_views[1]), global_valid, 1);
    assert(!knows1);
    assert(color1 == -1);

    const auto [knows2, color2] = hats::can_guess(
        parts[2].classes.at(ks.actual_views[2]), global_valid, 2);
    assert(!knows2);
    assert(color2 == -1);
}

void test_init_knowledge_limit_n_gt_20() {
    hats::Graph g(21);
    const hats::WorldMask actual = 1;

    try {
        (void)hats::init_knowledge(g, actual);
        assert(false);
    } catch (const std::invalid_argument&) {
        // Success
    }
}

void test_can_guess_empty_class_returns_false() {
    const std::vector<hats::WorldIndex> empty_cls{};
    hats::WorldSet global_valid(1);
    global_valid.set(0);

    const auto [knows, color] = hats::can_guess(empty_cls, global_valid, 0);
    assert(!knows);
    assert(color == -1);
}

int main() {
    test_init_knowledge_complete_graph_n3();
    test_can_guess_complete_graph_n3();
    test_init_knowledge_limit_n_gt_20();
    test_can_guess_empty_class_returns_false();

    std::cout << "All knowledge tests passed!" << std::endl;
    return 0;
}
