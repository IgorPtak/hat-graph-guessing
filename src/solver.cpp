#include "solver/solver.hpp"
#include "hats/graph.hpp"
#include "hats/knowledge.hpp"

namespace solver {

Solver::Solver(const hats::Graph &graph, hats::WorldMask actual_world)
    : graph_(graph),
      ks_(hats::init_knowledge(graph, actual_world)),
      parts_(hats::compute_partitions(graph, ks_.world_count)) {

    global_valid_worlds_.clear_all();
    for (std::size_t w = 1; w < ks_.world_count; w++) {
        global_valid_worlds_.set(static_cast<hats::WorldIndex>(w));
    }
}

bool Solver::apply_silence(const std::vector<bool> &already_guessed) {
    hats::WorldSet worlds_to_remove;
    worlds_to_remove.clear_all();
    bool any_reduced = false;

    // Iterate over equivalence classes per agent instead of per world.
    // All worlds sharing the same view (neighbors' bits) are indistinguishable to that agent,
    // so the "can guess?" answer is identical for the whole class — O(n * 2^n) total.
    for (std::size_t player = 0; player < ks_.n; player++) {
        if (already_guessed[player])
            continue;

        for (const auto &[view, worlds_in_class] : parts_[player].classes) {
            bool seen_zero = false;
            bool seen_one = false;

            for (const hats::WorldIndex w : worlds_in_class) {
                if (!global_valid_worlds_.test(w))
                    continue;
                if (hats::test_bit(static_cast<hats::WorldMask>(w),
                                   static_cast<hats::PlayerId>(player)))
                    seen_one = true;
                else
                    seen_zero = true;
                if (seen_zero && seen_one)
                    break;
            }

            // seen_zero != seen_one means all active worlds in the class agree on player's color
            if (seen_zero != seen_one) {
                for (const hats::WorldIndex w : worlds_in_class) {
                    if (global_valid_worlds_.test(w)) {
                        worlds_to_remove.set(w);
                        any_reduced = true;
                    }
                }
            }
        }
    }

    if (any_reduced) {
        global_valid_worlds_.subtract(worlds_to_remove, ks_.active_word_count);
        for (std::size_t player = 0; player < ks_.n; player++) {
            ks_.worlds[player].intersect_with(global_valid_worlds_, ks_.active_word_count);
        }
    }

    return any_reduced;
}

SimulationResult Solver::run() {
    int rounds = 0;
    std::vector<bool> already_guessed(ks_.n, false);
    int total_guess = 0;

    while (true) {
        bool guess_this_round = false;
        hats::WorldSet worlds_to_delete;
        worlds_to_delete.clear_all();

        for (std::size_t player = 0; player < ks_.n; player++) {
            if (already_guessed[player])
                continue;

            auto guess = hats::can_guess(ks_, static_cast<hats::PlayerId>(player));
            if (guess.first) {
                guess_this_round = true;
                already_guessed[player] = true;
                total_guess++;

                for (std::size_t w = 1; w < ks_.world_count; w++) {
                    if (!global_valid_worlds_.test(w))
                        continue;

                    int color_in_w =
                        hats::test_bit(static_cast<hats::WorldMask>(w), player) ? 1 : 0;
                    if (color_in_w != guess.second) {
                        worlds_to_delete.set(w);
                        continue;
                    }

                    // Check if player could actually guess in world w using precomputed partition.
                    const hats::VertexMask view =
                        static_cast<hats::WorldMask>(w) & parts_[player].neighbors_mask;
                    const auto &cls = parts_[player].classes.at(view);
                    bool cls_seen_zero = false;
                    bool cls_seen_one = false;
                    for (const hats::WorldIndex w2 : cls) {
                        if (!global_valid_worlds_.test(w2))
                            continue;
                        if (hats::test_bit(static_cast<hats::WorldMask>(w2),
                                           static_cast<hats::PlayerId>(player)))
                            cls_seen_one = true;
                        else
                            cls_seen_zero = true;
                        if (cls_seen_zero && cls_seen_one)
                            break;
                    }
                    if (cls_seen_zero == cls_seen_one)
                        worlds_to_delete.set(static_cast<hats::WorldIndex>(w));
                }
            }
        }
        if (total_guess == static_cast<int>(ks_.n)) {
            return {true, false, rounds};
        }

        if (guess_this_round) {
            global_valid_worlds_.subtract(worlds_to_delete, ks_.active_word_count);
            for (std::size_t player = 0; player < ks_.n; player++) {
                ks_.worlds[player].intersect_with(global_valid_worlds_, ks_.active_word_count);
            }
        } else {
            bool has_reduced = apply_silence(already_guessed);
            if (!has_reduced) {
                return {false, true, rounds};
            }
        }

        rounds++;
    }
}

} // namespace solver
