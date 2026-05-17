#include <cassert>
#include <iostream>

#include "hats/graph.hpp"
#include "hats/knowledge.hpp"
#include "solver/solver.hpp"

// ── K₃ — complete graph on 3 nodes ───────────────────────────────────────────
//
// Theory: in Kₙ with k red hats, all agents deduce in the same round.
// In our round counter the result is rounds = k   (k ≤ n-1)
// because each silence round eliminates one "level" of possible worlds.

// 1 red hat: agent with red sees 2 whites → singleton class → guesses immediately.
// Other agents learn from the guess, resolve in the same round counter value.
void test_solver_k3_1red() {
    hats::Graph g = hats::Graph::make_complete(3);
    solver::Solver s(g, 0b001);
    auto r = s.run();
    assert(r.success && !r.deadlock && r.rounds == 1);
}

// 2 red hats: one silence round needed before both red agents can deduce.
void test_solver_k3_2red() {
    hats::Graph g = hats::Graph::make_complete(3);
    solver::Solver s(g, 0b011);
    auto r = s.run();
    assert(r.success && !r.deadlock && r.rounds == 2);
}

// K₃ is vertex-transitive: rounds depend only on the number of red hats, not their positions.
void test_solver_k3_symmetry() {
    hats::Graph g = hats::Graph::make_complete(3);

    // All permutations of 1 red hat → rounds == 1
    for (hats::WorldMask actual : {0b001u, 0b010u, 0b100u}) {
        solver::Solver s(g, actual);
        auto r = s.run();
        assert(r.success && !r.deadlock && r.rounds == 1);
    }

    // All permutations of 2 red hats → rounds == 2
    for (hats::WorldMask actual : {0b011u, 0b101u, 0b110u}) {
        solver::Solver s(g, actual);
        auto r = s.run();
        assert(r.success && !r.deadlock && r.rounds == 2);
    }

    // 3 red hats: two silence rounds to distinguish from the 1- and 2-red cases.
    {
        solver::Solver s(g, 0b111);
        auto r = s.run();
        assert(r.success && !r.deadlock && r.rounds == 2);
    }
}

// ── K₂ — complete graph on 2 nodes ───────────────────────────────────────────
//
// Each agent sees the other directly. One silence round suffices regardless of
// how many red hats there are.

void test_solver_k2_1red() {
    hats::Graph g = hats::Graph::make_complete(2);
    // Both arrangements of 1 red hat
    for (hats::WorldMask actual : {0b01u, 0b10u}) {
        solver::Solver s(g, actual);
        auto r = s.run();
        assert(r.success && !r.deadlock && r.rounds == 1);
    }
}

void test_solver_k2_2red() {
    hats::Graph g = hats::Graph::make_complete(2);
    solver::Solver s(g, 0b11);
    auto r = s.run();
    assert(r.success && !r.deadlock && r.rounds == 1);
}

// ── Star S₃ — center (0) connected to leaves (1, 2) ─────────────────────────
//
// The leaves cannot see each other, which creates fundamental information
// asymmetry. The only solvable configuration is the one where the center
// is the sole red hat: it sees two whites → singleton class → guesses
// immediately; after the announcement, both leaves have a unique valid world.

void test_solver_star3_center_only_red() {
    hats::Graph g = hats::Graph::make_star(3);
    solver::Solver s(g, 0b001); // center = red, leaves = white
    auto r = s.run();
    assert(r.success && !r.deadlock && r.rounds == 1);
}

// One leaf red, center white: center cannot distinguish which of its views
// is realised; the red leaf cannot see the other leaf. Deadlock.
void test_solver_star3_one_leaf_red() {
    hats::Graph g = hats::Graph::make_star(3);
    for (hats::WorldMask actual : {0b010u, 0b100u}) {
        solver::Solver s(g, actual);
        auto r = s.run();
        assert(!r.success && r.deadlock);
    }
}

// Both leaves red (original deadlock test, kept as regression).
void test_solver_star3_both_leaves_red() {
    hats::Graph g = hats::Graph::make_star(3);
    solver::Solver s(g, 0b110);
    auto r = s.run();
    assert(!r.success && r.deadlock);
}

// Any configuration other than "center only red" also deadlocks in S₃.
void test_solver_star3_mixed_deadlock() {
    hats::Graph g = hats::Graph::make_star(3);
    // center+leaf1, center+leaf2, both leaves, all three
    for (hats::WorldMask actual : {0b011u, 0b101u, 0b110u, 0b111u}) {
        solver::Solver s(g, actual);
        auto r = s.run();
        assert(!r.success && r.deadlock);
    }
}

// ── Star S₄ — center (0) connected to leaves (1, 2, 3) ──────────────────────
//
// Same structural argument as S₃: only center-only-red is solvable.

void test_solver_star4_center_only_red() {
    hats::Graph g = hats::Graph::make_star(4);
    solver::Solver s(g, 0b0001); // center red, all leaves white
    auto r = s.run();
    assert(r.success && !r.deadlock && r.rounds == 1);
}

void test_solver_star4_any_leaf_red_deadlock() {
    hats::Graph g = hats::Graph::make_star(4);
    // Any world where at least one leaf is red → deadlock (leaves can't see each other).
    for (hats::WorldMask actual : {0b0010u, 0b0100u, 0b1000u, 0b0110u, 0b1010u}) {
        solver::Solver s(g, actual);
        auto r = s.run();
        assert(!r.success && r.deadlock);
    }
}

// ── Cycle C₄ ─────────────────────────────────────────────────────────────────
//
// In a 4-cycle (0-1-2-3-0) each agent sees two neighbours. Agents sitting
// opposite each other (0 and 2, or 1 and 3) cannot see each other at all,
// which can cause deadlocks similar to the star.

void test_solver_cycle4_1red() {
    // With a single red hat the holder sees two whites and deduces immediately.
    hats::Graph g = hats::Graph::make_cycle(4);
    for (hats::WorldMask actual : {0b0001u, 0b0010u, 0b0100u, 0b1000u}) {
        solver::Solver s(g, actual);
        auto r = s.run();
        assert(r.success && !r.deadlock);
    }
}

// ── K_n large — complete graphs ───────────────────────────────────────────────
//
// Theoretical result:  rounds = min(k, n-1)
//   - k silence rounds eliminate worlds with fewer red hats one level at a time
//   - when k = n (all red), n-1 levels must be eliminated → rounds = n-1

// 1 red hat: runs up to kMaxSupportedPlayers (currently 24) to validate the full range.
// Only 1 silence round happens, so despite the large partition build this is feasible.
void test_solver_complete_large_1_red() {
    for (std::size_t n = 3; n <= hats::kMaxSupportedPlayers; n++) {
        hats::Graph g = hats::Graph::make_complete(n);
        solver::Solver s(g, 1); // only agent 0 is red
        auto r = s.run();
        assert(r.success && !r.deadlock);
        assert(r.rounds == 1);
    }
}

// All red: each silence round eliminates all worlds with exactly j red hats
// (j = 1, 2, ..., n-1). After n-1 rounds only the all-red world remains.
// Capped at n=14 — n-1 rounds over 2^n worlds is expensive for larger n.
void test_solver_complete_large_all_red() {
    for (std::size_t n = 3; n <= 14; n++) {
        hats::Graph g = hats::Graph::make_complete(n);
        const hats::WorldMask all_red = (hats::WorldMask(1) << n) - 1;
        solver::Solver s(g, all_red);
        auto r = s.run();
        assert(r.success && !r.deadlock);
        assert(r.rounds == static_cast<int>(n - 1));
    }
}

// k = n/2 red hats: rounds = k = n/2  (min(k, n-1) = k since k < n for n >= 4).
// Capped at n=14 — n/2 rounds over 2^n worlds is expensive for larger n.
void test_solver_complete_large_half_red() {
    for (std::size_t n = 4; n <= 14; n++) {
        hats::Graph g = hats::Graph::make_complete(n);
        const hats::WorldMask half_red = (hats::WorldMask(1) << (n / 2)) - 1;
        solver::Solver s(g, half_red);
        auto r = s.run();
        assert(r.success && !r.deadlock);
        assert(r.rounds == static_cast<int>(n / 2));
    }
}

// ── S_n large — star graphs ───────────────────────────────────────────────────

// Center (player 0) sees all leaves white → singleton class → deduces in 1 round.
// Immediate success means the solver terminates after a single guess event,
// making this feasible up to n=20 despite the partition-build cost.
void test_solver_star_large_center_only_red() {
    for (std::size_t n = 3; n <= 20; n++) {
        hats::Graph g = hats::Graph::make_star(n);
        solver::Solver s(g, 1); // bit 0 = center red, all leaves white
        auto r = s.run();
        assert(r.success && !r.deadlock);
        assert(r.rounds == 1);
    }
}

// Any single leaf red (center white): center cannot determine own color;
// the red leaf cannot see other leaves. Structural deadlock.
void test_solver_star_large_leaf_red_deadlock() {
    for (std::size_t n = 3; n <= 20; n++) {
        hats::Graph g = hats::Graph::make_star(n);
        solver::Solver s(g, 2); // bit 1 = leaf 1 red, everything else white
        auto r = s.run();
        assert(!r.success && r.deadlock);
    }
}

// All leaves red (center white): same structural deadlock.
void test_solver_star_large_all_leaves_red_deadlock() {
    for (std::size_t n = 3; n <= 20; n++) {
        hats::Graph g = hats::Graph::make_star(n);
        const hats::WorldMask all_leaves = ((hats::WorldMask(1) << n) - 1) & ~hats::WorldMask(1);
        solver::Solver s(g, all_leaves);
        auto r = s.run();
        assert(!r.success && r.deadlock);
    }
}

// ── C_n large — cycle graphs ──────────────────────────────────────────────────
//
// C₃ = K₃ (triangle): always succeeds (covered by K₃ tests above).
//
// C_n for n ≥ 4: always deadlocks, for ANY non-zero actual world.
// Reason: C_n is vertex-transitive — the graph looks identical from every agent's
// perspective. For n ≥ 4 this creates an irreducible ambiguity: each agent can
// always find another consistent world where their own color is flipped, so no
// class ever becomes uniform.
//
// Verified empirically: ALL 15 non-zero worlds of C₄ deadlock at rounds = 0.

void test_solver_cycle_n4_always_deadlock() {
    hats::Graph g = hats::Graph::make_cycle(4);
    for (hats::WorldMask actual = 1; actual < 16; actual++) {
        solver::Solver s(g, actual);
        auto r = s.run();
        assert(!r.success && r.deadlock);
    }
}

void test_solver_cycle_large_deadlock() {
    // Test representative worlds (1 red, all red, half red) for n = 5..18.
    // Cycles deadlock immediately (0 silence rounds attempted), so only partition
    // build cost applies — feasible up to n=18 within reasonable time.
    for (std::size_t n = 5; n <= 18; n++) {
        hats::Graph g = hats::Graph::make_cycle(n);

        // 1 red
        {
            solver::Solver s(g, 1);
            auto r = s.run();
            assert(!r.success && r.deadlock);
        }

        // all red
        {
            const hats::WorldMask all = (hats::WorldMask(1) << n) - 1;
            solver::Solver s(g, all);
            auto r = s.run();
            assert(!r.success && r.deadlock);
        }

        // half red (agents 0..n/2-1 red)
        {
            const hats::WorldMask half = (hats::WorldMask(1) << (n / 2)) - 1;
            solver::Solver s(g, half);
            auto r = s.run();
            assert(!r.success && r.deadlock);
        }
    }
}

// ── W_n — wheel graphs ────────────────────────────────────────────────────────
//
// W_4 = K_4 (leaf cycle C_3 = K_3, center connects to all 3 → full K_4).
// W_n for n ≥ 5: only w = 2^(n-1) (center alone red) succeeds in 1 round.

static hats::Graph make_wheel(std::size_t n) {
    hats::Graph g(n);
    for (std::size_t i = 0; i < n - 1; i++) {
        g.add_edge(static_cast<hats::PlayerId>(i), static_cast<hats::PlayerId>((i + 1) % (n - 1)));
        g.add_edge(static_cast<hats::PlayerId>(i), static_cast<hats::PlayerId>(n - 1));
    }
    return g;
}

void test_solver_wheel_n4_equals_k4() {
    // W_4 has the same edges as K_4: all 15 worlds succeed.
    hats::Graph g = make_wheel(4);
    for (hats::WorldMask w = 1; w < 16; w++) {
        solver::Solver s(g, w);
        auto r = s.run();
        assert(r.success && !r.deadlock);
    }
}

void test_solver_wheel_center_only_red_succeeds() {
    // W_n (n=5..8): only w=2^(n-1) succeeds (center alone is red).
    for (std::size_t n = 5; n <= 8; n++) {
        hats::Graph g = make_wheel(n);
        const hats::WorldMask center_only = hats::WorldMask(1) << (n - 1);
        solver::Solver s(g, center_only);
        auto r = s.run();
        assert(r.success && !r.deadlock && r.rounds == 1);
    }
}

void test_solver_wheel_non_center_deadlocks() {
    // W_5: all worlds except w=16 deadlock.
    hats::Graph g = make_wheel(5);
    for (hats::WorldMask w = 1; w < 32; w++) {
        if (w == 16)
            continue; // the one success
        solver::Solver s(g, w);
        auto r = s.run();
        assert(!r.success && r.deadlock);
    }
}

// ── K_n \ e — K_n with one edge removed ──────────────────────────────────────
//
// Theorem: K_n \ {u, v} succeeds iff bit_u(w) = 0 AND bit_v(w) = 0.
// For n=5 removing {3,4}: exactly worlds 1..7 (bits 3,4 both 0) succeed.

static hats::Graph make_kn_minus_edge(std::size_t n, hats::PlayerId u, hats::PlayerId v) {
    hats::Graph g(n);
    for (hats::PlayerId a = 0; a < static_cast<hats::PlayerId>(n); a++) {
        for (hats::PlayerId b = static_cast<hats::PlayerId>(a + 1);
             b < static_cast<hats::PlayerId>(n); b++) {
            if ((a == u && b == v) || (a == v && b == u))
                continue;
            g.add_edge(a, b);
        }
    }
    return g;
}

void test_solver_kne_success_iff_both_white() {
    // K_5 \ {3,4}: worlds 1..7 (bit3=bit4=0) succeed; 8..31 deadlock.
    hats::Graph g = make_kn_minus_edge(5, 3, 4);
    for (hats::WorldMask w = 1; w < 32; w++) {
        const bool both_white = !((w >> 3) & 1) && !((w >> 4) & 1);
        solver::Solver s(g, w);
        auto r = s.run();
        if (both_white) {
            assert(r.success && !r.deadlock);
        } else {
            assert(!r.success && r.deadlock);
        }
    }
}

void test_solver_kne_rounds_formula() {
    // For K_5 \ {3,4} and world w with bit3=bit4=0, rounds = k = popcount(w).
    hats::Graph g = make_kn_minus_edge(5, 3, 4);
    for (hats::WorldMask w = 1; w <= 7; w++) { // all have bit3=bit4=0
        const int k = __builtin_popcountll(w);
        solver::Solver s(g, w);
        auto r = s.run();
        assert(r.success && r.rounds == k);
    }
}

void test_solver_kne_larger() {
    // K_6 \ {4,5}: worlds where bit4=bit5=0 (i.e. 1..15) succeed.
    hats::Graph g = make_kn_minus_edge(6, 4, 5);
    for (hats::WorldMask w = 1; w < 64; w++) {
        const bool both_white = !((w >> 4) & 1) && !((w >> 5) & 1);
        solver::Solver s(g, w);
        auto r = s.run();
        if (both_white) {
            assert(r.success && !r.deadlock);
        } else {
            assert(!r.success && r.deadlock);
        }
    }
}

// ── O_n : ordered visibility (directed) ──────────────────────────────────────

void test_solver_ordered_only_w1_succeeds() {
    // For O_n, only w=1 (vertex 0 alone red) succeeds; all others deadlock.
    for (std::size_t n = 2; n <= 8; ++n) {
        hats::Graph g = hats::Graph::make_ordered_visibility(n);
        const hats::WorldMask total = (hats::WorldMask{1} << n) - 1;
        for (hats::WorldMask w = 1; w <= total; ++w) {
            solver::Solver s(g, w);
            auto r = s.run();
            if (w == 1) {
                assert(r.success && !r.deadlock && r.rounds == 1);
            } else {
                assert(!r.success && r.deadlock);
            }
        }
    }
}

int main() {
    // K₃ — small (regression)
    test_solver_k3_1red();
    test_solver_k3_2red();
    test_solver_k3_symmetry();

    // K₂ — small (regression)
    test_solver_k2_1red();
    test_solver_k2_2red();

    // S₃ — small (regression)
    test_solver_star3_center_only_red();
    test_solver_star3_one_leaf_red();
    test_solver_star3_both_leaves_red();
    test_solver_star3_mixed_deadlock();

    // S₄ — small (regression)
    test_solver_star4_center_only_red();
    test_solver_star4_any_leaf_red_deadlock();

    // K_n large
    test_solver_complete_large_1_red();
    test_solver_complete_large_all_red();
    test_solver_complete_large_half_red();

    // S_n large
    test_solver_star_large_center_only_red();
    test_solver_star_large_leaf_red_deadlock();
    test_solver_star_large_all_leaves_red_deadlock();

    // C_n large
    test_solver_cycle_n4_always_deadlock();
    test_solver_cycle_large_deadlock();

    // W_n — wheel graphs
    test_solver_wheel_n4_equals_k4();
    test_solver_wheel_center_only_red_succeeds();
    test_solver_wheel_non_center_deadlocks();

    // K_n \ e — near-complete graphs
    test_solver_kne_success_iff_both_white();
    test_solver_kne_rounds_formula();
    test_solver_kne_larger();

    // O_n — ordered visibility (directed)
    test_solver_ordered_only_w1_succeeds();

    std::cout << "All solver tests passed!\n";
    return 0;
}
