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

// 1 red hat: the red agent sees all whites → singleton class → guesses immediately.
// After the announcement the remaining agents have a unique valid world. rounds = 1.
void test_solver_complete_large_1_red() {
    for (std::size_t n = 3; n <= 12; n++) {
        hats::Graph g = hats::Graph::make_complete(n);
        solver::Solver s(g, 1); // only agent 0 is red
        auto r = s.run();
        assert(r.success && !r.deadlock);
        assert(r.rounds == 1);
    }
}

// All red: each silence round eliminates all worlds with exactly j red hats
// (j = 1, 2, ..., n-1). After n-1 rounds only the all-red world remains.
void test_solver_complete_large_all_red() {
    for (std::size_t n = 3; n <= 12; n++) {
        hats::Graph g = hats::Graph::make_complete(n);
        const hats::WorldMask all_red = (hats::WorldMask(1) << n) - 1;
        solver::Solver s(g, all_red);
        auto r = s.run();
        assert(r.success && !r.deadlock);
        assert(r.rounds == static_cast<int>(n - 1));
    }
}

// k = n/2 red hats: rounds = k = n/2  (min(k, n-1) = k since k < n for n >= 4).
void test_solver_complete_large_half_red() {
    for (std::size_t n = 4; n <= 12; n++) {
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
void test_solver_star_large_center_only_red() {
    for (std::size_t n = 3; n <= 12; n++) {
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
    for (std::size_t n = 3; n <= 12; n++) {
        hats::Graph g = hats::Graph::make_star(n);
        solver::Solver s(g, 2); // bit 1 = leaf 1 red, everything else white
        auto r = s.run();
        assert(!r.success && r.deadlock);
    }
}

// All leaves red (center white): same structural deadlock.
void test_solver_star_large_all_leaves_red_deadlock() {
    for (std::size_t n = 3; n <= 12; n++) {
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
    // Test representative worlds (1 red, all red, half red) for n = 5..10.
    for (std::size_t n = 5; n <= 10; n++) {
        hats::Graph g = hats::Graph::make_cycle(n);

        // 1 red
        { solver::Solver s(g, 1); auto r = s.run(); assert(!r.success && r.deadlock); }

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

    std::cout << "All solver tests passed!\n";
    return 0;
}
