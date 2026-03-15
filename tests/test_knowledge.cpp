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

    assert(ks.n == 3);

    // Player 0 sees players 1 and 2 as 0,0. With w != 0 only world 001 remains.
    assert(ks.worlds[0].count(ks.active_word_count) == 1);

    // Player 1 sees (player 0 = 1, player 2 = 0), so own bit can be 0 or 1.
    assert(ks.worlds[1].count(ks.active_word_count) == 2);

    // Player 2 sees (player 0 = 1, player 1 = 0), so own bit can be 0 or 1.
    assert(ks.worlds[2].count(ks.active_word_count) == 2);
}

void test_can_guess_complete_graph_n3() {
    hats::Graph g(3);
    g.add_edge(0, 1);
    g.add_edge(0, 2);
    g.add_edge(1, 2);

    const hats::WorldMask actual = 0b001;
    const hats::KnowledgeState ks = hats::init_knowledge(g, actual);

    const auto [knows0, color0] = hats::can_guess(ks, 0);
    assert(knows0);
    assert(color0 == 1);

    const auto [knows1, color1] = hats::can_guess(ks, 1);
    assert(!knows1);
    assert(color1 == -1);

    const auto [knows2, color2] = hats::can_guess(ks, 2);
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

void test_can_guess_out_of_range_player() {
    hats::Graph g(3);
    g.add_edge(0, 1);
    g.add_edge(0, 2);
    g.add_edge(1, 2);

    const hats::KnowledgeState ks = hats::init_knowledge(g, 0b001);

    try {
        (void)hats::can_guess(ks, 7);
        assert(false);
    } catch (const std::out_of_range&) {
        // Success
    }
}

int main() {
    test_init_knowledge_complete_graph_n3();
    test_can_guess_complete_graph_n3();
    test_init_knowledge_limit_n_gt_20();
    test_can_guess_out_of_range_player();

    std::cout << "All knowledge tests passed!" << std::endl;
    return 0;
}
