#include <cassert>
#include <iostream>
#include "hats/graph.hpp"

void test_basic_graph() {
    hats::Graph g(5);
    assert(g.size() == 5);
    
    assert(g.add_edge(0, 1));
    assert(g.add_edge(0, 4));
    assert(!g.add_edge(0, 1));
    
    assert(g.has_edge(0, 1));
    assert(g.has_edge(1, 0));
    assert(g.has_edge(0, 4));
    assert(g.has_edge(4, 0));
    assert(!g.has_edge(1, 4));
    
    assert(g.neighbors(0) == ((1ULL << 1) | (1ULL << 4)));
    assert(g.degree(0) == 2);
}

void test_self_loop() {
    hats::Graph g(5);
    try {
        (void)g.add_edge(0, 0);
        assert(false);
    } catch (const std::invalid_argument&) {
        // Success
    }
    assert(!g.has_edge(0, 0));
    assert(g.neighbors(0) == 0);
}

void test_out_of_range() {
    hats::Graph g(5);
    try {
        g.add_edge(0, 5);
        assert(false);
    } catch (const std::out_of_range&) {
        // Success
    }
}

void test_invalid_vertex_in_neighbors() {
    hats::Graph g(5);

    try {
        (void)g.neighbors(255);
        assert(false);
    } catch (const std::out_of_range&) {
        // Success
    }

    try {
        (void)g.add_edge(1, 1);
        assert(false);
    } catch (const std::invalid_argument&) {
        // Success
    }
}

void test_graph_size_validation() {
    try {
        hats::Graph g0(0);
        assert(false);
    } catch (const std::invalid_argument&) {
        // Success
    }

    try {
        hats::Graph g65(65);
        assert(false);
    } catch (const std::invalid_argument&) {
    }
}

int main() {
    test_basic_graph();
    test_self_loop();
    test_out_of_range();
    test_invalid_vertex_in_neighbors(); 
    test_graph_size_validation();

    std::cout << "All graph tests passed!" << std::endl;
    return 0;
}
