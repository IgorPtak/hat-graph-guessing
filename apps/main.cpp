#include <iostream>
#include "hats/graph.hpp"

int main() {
    std::cout << "Hats Application Smoke Test" << std::endl;
    
    hats::Graph g(10);
    g.add_edge(0, 1);
    g.add_edge(0, 2);
    
    std::cout << "Neighbors of 0: " << g.neighbors(0) << std::endl;
    if (g.has_edge(0, 1) && g.has_edge(0, 2) && !g.has_edge(0, 3)) {
        std::cout << "Basic graph operations work!" << std::endl;
    } else {
        std::cerr << "Graph operations failed!" << std::endl;
        return 1;
    }
    
    return 0;
}
