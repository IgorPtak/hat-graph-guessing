#pragma once

#include "hats/types.hpp"
#include <vector>
#include <array>

namespace hats {

// Undirected simple graph, neighbors as bit masks.
class Graph final { 
public: 
    explicit Graph(std::size_t n); 

    [[nodiscard]] std::size_t size() const noexcept; 

    bool add_edge(PlayerId u, PlayerId v); 

    [[nodiscard]] VertexMask neighbors(PlayerId v) const;
    [[nodiscard]] bool has_edge(PlayerId u, PlayerId v) const;
    [[nodiscard]] std::size_t degree(PlayerId v) const;

    [[nodiscard]] static Graph make_complete(std::size_t n);
    [[nodiscard]] static Graph make_cycle(std::size_t n);
    [[nodiscard]] static Graph make_star(std::size_t n);

private: 
    std::size_t n_{0};
    std::array<VertexMask, kMaxPlayers> adj_{};

    [[nodiscard]] bool is_valid_vertex(PlayerId v) const noexcept;
    void validate_vertex(PlayerId v) const;
};

} // namespace hats
