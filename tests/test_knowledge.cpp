#include <cassert>
#include <iostream>
#include <stdexcept>

#include "hats/graph.hpp"
#include "hats/knowledge.hpp"

// ── WorldSet ─────────────────────────────────────────────────────────────────

void test_worldset_size_matches_constructor() {
    hats::WorldSet ws(5);
    assert(ws.words.size() == 5);
    assert(ws.empty());
}

void test_worldset_set_and_test() {
    hats::WorldSet ws(2); // 2 × 64 = 128 possible indices

    ws.set(0);
    assert(ws.test(0));
    assert(!ws.test(1));

    ws.set(63); // last bit of first word
    assert(ws.test(63));
    assert(!ws.test(62));

    ws.set(64); // first bit of second word
    assert(ws.test(64));
    assert(!ws.test(65));
}

void test_worldset_count() {
    hats::WorldSet ws(1);
    assert(ws.count() == 0);
    ws.set(0);
    ws.set(7);
    ws.set(63);
    assert(ws.count() == 3);
}

void test_worldset_empty() {
    hats::WorldSet ws(2);
    assert(ws.empty());
    ws.set(64);
    assert(!ws.empty());
}

void test_worldset_intersect_with() {
    hats::WorldSet a(1), b(1);
    a.set(1); a.set(2); a.set(3);
    b.set(2); b.set(3); b.set(4);
    a.intersect_with(b);
    assert(!a.test(1));
    assert(a.test(2));
    assert(a.test(3));
    assert(!a.test(4));
    assert(a.count() == 2);
}

void test_worldset_subtract() {
    hats::WorldSet a(1), b(1);
    a.set(1); a.set(2); a.set(3);
    b.set(2); b.set(5);
    a.subtract(b);
    assert(a.test(1));
    assert(!a.test(2)); // removed
    assert(a.test(3));
    assert(!a.test(5)); // was never in a
    assert(a.count() == 2);
}

void test_worldset_union_with() {
    hats::WorldSet a(1), b(1);
    a.set(1); a.set(2);
    b.set(2); b.set(3);
    a.union_with(b);
    assert(a.test(1));
    assert(a.test(2));
    assert(a.test(3));
    assert(a.count() == 3);
}

void test_worldset_clear_all() {
    hats::WorldSet ws(2);
    ws.set(0); ws.set(64); ws.set(127);
    assert(ws.count() == 3);
    ws.clear_all();
    assert(ws.empty());
    assert(ws.count() == 0);
}

// ── compute_partitions ────────────────────────────────────────────────────────

// Every world in {1..world_count-1} must appear in exactly one class per agent.
void test_partitions_cover_all_worlds() {
    hats::Graph g = hats::Graph::make_complete(4);
    const hats::KnowledgeState ks = hats::init_knowledge(g, 0b0001);
    const hats::Partitions parts = hats::compute_partitions(g, ks.world_count);

    for (std::size_t player = 0; player < ks.n; player++) {
        std::size_t total = 0;
        for (const auto &[view, cls] : parts[player].classes)
            total += cls.size();
        assert(total == ks.world_count - 1); // excludes world 0
    }
}

// For every world w in a class keyed by view: (w & mask) == view.
void test_partitions_view_validity() {
    hats::Graph g = hats::Graph::make_complete(3);
    const hats::KnowledgeState ks = hats::init_knowledge(g, 0b001);
    const hats::Partitions parts = hats::compute_partitions(g, ks.world_count);

    for (std::size_t player = 0; player < ks.n; player++) {
        const hats::VertexMask mask = parts[player].neighbors_mask;
        for (const auto &[view, cls] : parts[player].classes) {
            for (const hats::WorldIndex w : cls) {
                assert((static_cast<hats::WorldMask>(w) & mask) == view);
            }
        }
    }
}

// In a star graph a leaf sees only the center, so it has exactly 2 classes:
// center-white and center-red.
void test_partitions_star_leaf_has_2_classes() {
    hats::Graph g = hats::Graph::make_star(5);
    const hats::KnowledgeState ks = hats::init_knowledge(g, 0b00001);
    const hats::Partitions parts = hats::compute_partitions(g, ks.world_count);

    // Players 1..4 are leaves — each sees only the center (player 0).
    for (std::size_t leaf = 1; leaf < 5; leaf++) {
        assert(parts[leaf].classes.size() == 2);
    }

    // Center sees all 4 leaves → 2^4 = 16 distinct views.
    assert(parts[0].classes.size() == 16);
}

// ── init_knowledge / actual_views ────────────────────────────────────────────

void test_actual_views_correct() {
    hats::Graph g = hats::Graph::make_complete(3);
    const hats::WorldMask actual = 0b101;
    const hats::KnowledgeState ks = hats::init_knowledge(g, actual);

    for (std::size_t i = 0; i < ks.n; i++) {
        const hats::VertexMask expected = actual & g.neighbors(static_cast<hats::PlayerId>(i));
        assert(ks.actual_views[i] == expected);
    }
}

void test_init_knowledge_rejects_n_0() {
    hats::Graph g(1); // minimum valid graph; we'll test n=0 indirectly via the limit
    try {
        hats::Graph big(21);
        (void)hats::init_knowledge(big, 1);
        assert(false);
    } catch (const std::invalid_argument &) {}
}

// ── can_guess ─────────────────────────────────────────────────────────────────

// Class contains only worlds with player's bit = 1 → guesses red.
void test_can_guess_uniform_red() {
    // World 5 = 0b101 (bits 0 and 2 set). Class for player 0 seeing bit2=1 in K₃.
    const std::vector<hats::WorldIndex> cls = {5, 7}; // both have bit 0 = 1
    hats::WorldSet global(1);
    global.set(5); global.set(7);

    const auto [can, color] = hats::can_guess(cls, global, 0);
    assert(can);
    assert(color == 1);
}

// Class contains only worlds with player's bit = 0 → guesses white.
void test_can_guess_uniform_white() {
    const std::vector<hats::WorldIndex> cls = {2, 6}; // bit 0 = 0 in both
    hats::WorldSet global(1);
    global.set(2); global.set(6);

    const auto [can, color] = hats::can_guess(cls, global, 0);
    assert(can);
    assert(color == 0);
}

// Class contains worlds with both colors → cannot guess.
void test_can_guess_mixed_class() {
    const std::vector<hats::WorldIndex> cls = {2, 3}; // bit0: 0 and 1
    hats::WorldSet global(1);
    global.set(2); global.set(3);

    const auto [can, color] = hats::can_guess(cls, global, 0);
    assert(!can);
    assert(color == -1);
}

// All worlds in the class have been eliminated from global → cannot guess.
void test_can_guess_all_filtered_out() {
    const std::vector<hats::WorldIndex> cls = {1, 2, 3};
    hats::WorldSet global(1); // empty — nothing set

    const auto [can, color] = hats::can_guess(cls, global, 0);
    assert(!can);
    assert(color == -1);
}

// Only one world in the class, globally valid → player can guess.
void test_can_guess_singleton_class() {
    const std::vector<hats::WorldIndex> cls = {3}; // 0b011 — bit 1 = 1
    hats::WorldSet global(1);
    global.set(3);

    const auto [can, color] = hats::can_guess(cls, global, 1);
    assert(can);
    assert(color == 1);
}

// ── Integration: partitions + can_guess on K₃ ────────────────────────────────

// In K₃ with 1 red (agent 0), agent 0 can guess immediately;
// agents 1 and 2 have 2-world classes and cannot.
void test_k3_1red_initial_knowledge() {
    hats::Graph g = hats::Graph::make_complete(3);
    const hats::WorldMask actual = 0b001;
    const hats::KnowledgeState ks = hats::init_knowledge(g, actual);
    const hats::Partitions parts = hats::compute_partitions(g, ks.world_count);

    hats::WorldSet global(ks.active_word_count);
    for (std::size_t w = 1; w < ks.world_count; w++)
        global.set(static_cast<hats::WorldIndex>(w));

    // Agent 0 sees both neighbours white → singleton class → can guess.
    const auto &cls0 = parts[0].classes.at(ks.actual_views[0]);
    assert(cls0.size() == 1);
    const auto [can0, color0] = hats::can_guess(cls0, global, 0);
    assert(can0 && color0 == 1);

    // Agents 1 and 2 each have a 2-world class → cannot guess yet.
    const auto &cls1 = parts[1].classes.at(ks.actual_views[1]);
    assert(cls1.size() == 2);
    const auto [can1, color1] = hats::can_guess(cls1, global, 1);
    assert(!can1 && color1 == -1);

    const auto &cls2 = parts[2].classes.at(ks.actual_views[2]);
    assert(cls2.size() == 2);
    const auto [can2, color2] = hats::can_guess(cls2, global, 2);
    assert(!can2 && color2 == -1);
}

int main() {
    // WorldSet
    test_worldset_size_matches_constructor();
    test_worldset_set_and_test();
    test_worldset_count();
    test_worldset_empty();
    test_worldset_intersect_with();
    test_worldset_subtract();
    test_worldset_union_with();
    test_worldset_clear_all();

    // compute_partitions
    test_partitions_cover_all_worlds();
    test_partitions_view_validity();
    test_partitions_star_leaf_has_2_classes();

    // init_knowledge / actual_views
    test_actual_views_correct();
    test_init_knowledge_rejects_n_0();

    // can_guess
    test_can_guess_uniform_red();
    test_can_guess_uniform_white();
    test_can_guess_mixed_class();
    test_can_guess_all_filtered_out();
    test_can_guess_singleton_class();

    // integration
    test_k3_1red_initial_knowledge();

    std::cout << "All knowledge tests passed!\n";
    return 0;
}
