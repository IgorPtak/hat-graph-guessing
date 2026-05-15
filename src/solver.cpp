#include "solver/solver.hpp"
#include "hats/graph.hpp"
#include "hats/knowledge.hpp"

#ifdef _OPENMP
#include <omp.h>
#endif

namespace {
#ifdef _OPENMP
inline int max_threads() noexcept { return omp_get_max_threads(); }
inline int thread_id() noexcept { return omp_get_thread_num(); }
#else
inline int max_threads() noexcept { return 1; }
inline int thread_id() noexcept { return 0; }
#endif
} // namespace

namespace solver {

Solver::Solver(const hats::Graph &graph, hats::WorldMask actual_world)
    : graph_(graph),
      ks_(hats::init_knowledge(graph, actual_world)),
      parts_(hats::compute_partitions(graph, ks_.world_count)),
      global_valid_worlds_(ks_.active_word_count) {

    for (std::size_t w = 1; w < ks_.world_count; w++) {
        global_valid_worlds_.set(static_cast<hats::WorldIndex>(w));
    }
}

bool Solver::apply_silence(const std::vector<bool> &already_guessed) {
    const int nthreads = max_threads();
    std::vector<hats::WorldSet> thread_remove(
        static_cast<std::size_t>(nthreads), hats::WorldSet(ks_.active_word_count));

    // Each thread accumulates worlds-to-remove for the players it processes.
    // global_valid_worlds_ is read-only during this section.
    #pragma omp parallel for schedule(dynamic, 1)
    for (std::size_t player = 0; player < ks_.n; player++) {
        if (already_guessed[player])
            continue;

        hats::WorldSet &local = thread_remove[static_cast<std::size_t>(thread_id())];

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

            // Homogeneous active class: player would guess → silence eliminates it.
            if (seen_zero != seen_one) {
                for (const hats::WorldIndex w : worlds_in_class) {
                    if (global_valid_worlds_.test(w))
                        local.set(w);
                }
            }
        }
    }

    hats::WorldSet worlds_to_remove(ks_.active_word_count);
    for (int t = 0; t < nthreads; t++)
        worlds_to_remove.union_with(thread_remove[static_cast<std::size_t>(t)]);

    const bool any_reduced = !worlds_to_remove.empty();
    if (any_reduced)
        global_valid_worlds_.subtract(worlds_to_remove);

    return any_reduced;
}

SimulationResult Solver::run() {
    int rounds = 0;
    std::vector<bool> already_guessed(ks_.n, false);
    int total_guess = 0;

    struct Guesser {
        std::size_t player;
        int color;
    };

    while (true) {
        // Collect all guessers for this round (sequential: O(n × avg_class_size)).
        std::vector<Guesser> guessers;
        for (std::size_t player = 0; player < ks_.n; player++) {
            if (already_guessed[player])
                continue;

            const hats::VertexMask actual_view = ks_.actual_views[player];
            const auto &actual_cls = parts_[player].classes.at(actual_view);
            auto guess = hats::can_guess(actual_cls, global_valid_worlds_,
                                         static_cast<hats::PlayerId>(player));
            if (guess.first) {
                guessers.push_back({player, guess.second});
                already_guessed[player] = true;
                total_guess++;
            }
        }

        if (total_guess == static_cast<int>(ks_.n))
            return {true, false, rounds};

        if (!guessers.empty()) {
            // Parallel sweep over worlds: each thread marks worlds inconsistent
            // with this round's guesses into its own local WorldSet.
            // global_valid_worlds_ and parts_ are read-only during this section.
            const int nthreads = max_threads();
            std::vector<hats::WorldSet> thread_delete(
                static_cast<std::size_t>(nthreads), hats::WorldSet(ks_.active_word_count));

            #pragma omp parallel for schedule(static)
            for (std::size_t w = 1; w < ks_.world_count; w++) {
                if (!global_valid_worlds_.test(w))
                    continue;

                hats::WorldSet &local = thread_delete[static_cast<std::size_t>(thread_id())];
                bool should_delete = false;

                for (const auto &[gp, gc] : guessers) {
                    if (should_delete) break;

                    const int color_in_w =
                        hats::test_bit(static_cast<hats::WorldMask>(w),
                                       static_cast<hats::PlayerId>(gp)) ? 1 : 0;
                    if (color_in_w != gc) {
                        should_delete = true;
                        break;
                    }

                    // World w is only consistent with gp's guess if gp's class for
                    // view w is homogeneous (gp could actually guess in world w).
                    const hats::VertexMask view =
                        static_cast<hats::WorldMask>(w) & parts_[gp].neighbors_mask;
                    const auto &view_cls = parts_[gp].classes.at(view);
                    bool cls_seen_zero = false;
                    bool cls_seen_one = false;
                    for (const hats::WorldIndex w2 : view_cls) {
                        if (!global_valid_worlds_.test(w2)) continue;
                        if (hats::test_bit(static_cast<hats::WorldMask>(w2),
                                           static_cast<hats::PlayerId>(gp)))
                            cls_seen_one = true;
                        else
                            cls_seen_zero = true;
                        if (cls_seen_zero && cls_seen_one) break;
                    }
                    if (cls_seen_zero == cls_seen_one)
                        should_delete = true;
                }

                if (should_delete)
                    local.set(static_cast<hats::WorldIndex>(w));
            }

            hats::WorldSet worlds_to_delete(ks_.active_word_count);
            for (int t = 0; t < nthreads; t++)
                worlds_to_delete.union_with(thread_delete[static_cast<std::size_t>(t)]);
            global_valid_worlds_.subtract(worlds_to_delete);
        } else {
            bool has_reduced = apply_silence(already_guessed);
            if (!has_reduced)
                return {false, true, rounds};
        }

        rounds++;
    }
}

} // namespace solver
