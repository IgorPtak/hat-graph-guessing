#pragma once

#include <array> 
#include <cstdint> 
#include <limits> 
#include <stdexcept> 
#include <type_traits> 

namespace hats { 
    using WorldMask  = std::uint64_t;   // hats sequence mask N <= 64
    using VertexMask = std::uint64_t;   // vertex set mask 
    using PlayerId   = std::uint8_t;    // 0..63
    using Tick       = std::uint32_t;

    inline constexpr std::size_t kMaxPlayers = 64; 

    [[nodiscard]] constexpr VertexMask bit(PlayerId v) noexcept { 
        return VertexMask{1} << v;
    }

    [[nodiscard]] constexpr bool test_bit(VertexMask m, PlayerId v) noexcept { 
        return (m & bit(v)) != 0; 
    }

    constexpr void validate_player_count(std::size_t n) { 
        if (n == 0 || n > kMaxPlayers) { 
            throw std::invalid_argument("player count must be in [1, 64]"); 
        }
    }
}