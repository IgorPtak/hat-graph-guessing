#include "hats/graph.hpp"

#include <stdexcept>

namespace hats {

Graph::Graph(std::size_t n) : n_{n} {
    validate_player_count(n_);
    adj_.fill(0);
}

std::size_t Graph::size() const noexcept {
    return n_;
}

bool Graph::add_edge(PlayerId u, PlayerId v) {
    validate_vertex(u);
    validate_vertex(v);
    if (u == v) {
        throw std::invalid_argument("self-loops are not allowed");
    }

    const VertexMask edge_bit_u = bit(v);
    const bool already_exists = (adj_[u] & edge_bit_u) != 0;
    if (already_exists) {
        return false;
    }

    adj_[u] |= edge_bit_u;
    adj_[v] |= bit(u);
    return true;
}

VertexMask Graph::neighbors(PlayerId v) const {
    validate_vertex(v);
    return adj_[v];
}

bool Graph::has_edge(PlayerId u, PlayerId v) const {
    validate_vertex(u);
    validate_vertex(v);
    return (adj_[u] & bit(v)) != 0;
}

std::size_t Graph::degree(PlayerId v) const {
    validate_vertex(v);
    return static_cast<std::size_t>(__builtin_popcountll(adj_[v]));
}

bool Graph::is_valid_vertex(PlayerId v) const noexcept {
    return static_cast<std::size_t>(v) < n_;
}

void Graph::validate_vertex(PlayerId v) const {
    if (!is_valid_vertex(v)) {
        throw std::out_of_range("vertex index out of range");
    }
}

Graph Graph::make_complete(std::size_t n) {
    Graph g(n);
    for (PlayerId u = 0; u < n; u++) {
        for (PlayerId v = u + 1; v < n; v++) {
            g.add_edge(u, v);
        }
    }

    return g;
}

Graph Graph::make_cycle(std::size_t n) {
    if (n < 3) {
        throw std::invalid_argument("Cycle graph must have at least 3 vertices.");
    }
    Graph g(n);
    for (PlayerId u = 0; u < n; u++) {
        g.add_edge(u, (u + 1) % n);
    }

    return g;
}

Graph Graph::make_start(std::size_t n) {
    if (n < 2) {
        throw std::invalid_argument("Cycle graph must have at least 2 vertices.");
    }
    Graph g(n);
    for (PlayerId u = 1; u < n; u++) {
        g.add_edge(0, u);
    }

    return g;
}

} // namespace hats


