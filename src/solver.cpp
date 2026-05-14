#include "solver/solver.hpp"
#include "hats/graph.hpp"
#include "hats/knowledge.hpp"

namespace solver {

Solver::Solver(const hats::Graph &graph, hats::WorldMask actual_world)
    : graph_(graph),
    ks_(hats::init_knowledge(graph, actual_world)) {

        global_valid_worlds_.clear_all();

        for (std::size_t w = 1; w < ks_.world_count; w++) {
            global_valid_worlds_.set(static_cast<hats::WorldIndex>(w));
        }
    }

bool Solver::apply_silence(const std::vector<bool>& already_guessed) {
    hats::WorldSet worlds_to_remove;
    worlds_to_remove.clear_all();
    bool any_reduced = false;

    for (std::size_t w = 1; w < ks_.world_count; w++) {
        if (!global_valid_worlds_.test(static_cast<hats::WorldIndex>(w))) {
            continue;
        }

        hats::KnowledgeState hypothetical_ks = hats::init_knowledge(graph_, static_cast<hats::WorldMask>(w));

        for (std::size_t player = 0; player < ks_.n; player++) {
            if (already_guessed[player]) continue;
            hypothetical_ks.worlds[player].intersect_with(global_valid_worlds_, ks_.world_count);
        }

        bool somebody_guesses = false;
        for (std::size_t player = 0; player < ks_.n; player++) {
            if (already_guessed[player]) continue;
            if (hats::can_guess(hypothetical_ks, static_cast<hats::PlayerId>(player)).first) {
                somebody_guesses = true;
                break;
            }
        }

        if (somebody_guesses) {
            worlds_to_remove.set(static_cast<hats::WorldIndex>(w));
            any_reduced = true;
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


// Version with rounds required for all players to guess is yet to be implemented
// For now we get information when the first guess appear
// SimulationResult Solver::run() {
//     int rounds = 0;

//     while (true) {
//         int guess_count = 0;

//         for (std::size_t player = 0; player < ks_.n; player++) {
//             if (hats::can_guess(ks_, static_cast<hats::PlayerId>(player)).first) {
//                 guess_count++;
//             }
//         }

//         if (guess_count > 0) {
//             return {true, false, rounds};
//         }

//         bool has_worlds_reduced = apply_silence();

//         if (!has_worlds_reduced) {
//             return {false, true, rounds};
//         }

//         rounds++;
//     }
// }

SimulationResult Solver::run() {
    int rounds = 0;
    std::vector<bool> already_guessed(ks_.n, false);
    int total_guess = 0;

    while (true) {
        bool guess_this_round = false;
        hats::WorldSet worlds_to_delete;
        worlds_to_delete.clear_all();

        for (std::size_t player = 0; player < ks_.n; player++) {
            if (already_guessed[player]) continue;

            auto guess = hats::can_guess(ks_, static_cast<hats::PlayerId>(player));
            if (guess.first) {
                guess_this_round = true;
                already_guessed[player] = true;
                total_guess++;

                for (std::size_t w = 1; w < ks_.world_count; w++) {
                    if (!global_valid_worlds_.test(w)) continue;

                    int color_in_w = hats::test_bit(static_cast<hats::WorldMask>(w), player) ? 1 : 0;
                    if (color_in_w != guess.second) {
                        worlds_to_delete.set(w);
                        continue;
                    }

                    hats::KnowledgeState hypothetical_ks = hats::init_knowledge(graph_, static_cast<hats::WorldMask>(w));
                    hypothetical_ks.worlds[player].intersect_with(global_valid_worlds_, ks_.world_count);

                    if (!hats::can_guess(hypothetical_ks, static_cast<hats::PlayerId>(player)).first) {
                        worlds_to_delete.set(static_cast<hats::WorldIndex>(w));
                    }
                }
            }
        }
        if (total_guess == ks_.n) {
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

}
