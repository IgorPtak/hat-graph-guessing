#!/usr/bin/env python3
"""Animacja krok-po-kroku symulacji HATS (DEL).

Użycie:
  python3 visualize.py --topology complete --n 3 --world 0b011
  python3 visualize.py --topology star --n 4 --world 0b110 --save
"""
import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent / "build"))

import hats_py
import networkx as nx
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import numpy as np


def make_graph(topology: str, n: int) -> hats_py.Graph:
    if topology == "complete":
        return hats_py.Graph.make_complete(n)
    if topology == "cycle":
        return hats_py.Graph.make_cycle(n)
    return hats_py.Graph.make_star(n)


def parse_args():
    p = argparse.ArgumentParser(description="Animacja symulacji kapeluszy DEL")
    p.add_argument("--topology", choices=["complete", "cycle", "star"], default="complete")
    p.add_argument("--n", type=int, default=3)
    p.add_argument("--world", type=lambda x: int(x, 0), default=0b011,
                   help="Aktualny świat jako bitmaska (0b..., 0x..., lub dec)")
    p.add_argument("--save", action="store_true", help="Zapisz animację jako hats_simulation.gif")
    return p.parse_args()


def main():
    args = parse_args()
    n = args.n
    actual_world = args.world

    g = make_graph(args.topology, n)

    solver = hats_py.Solver(g, actual_world)
    trace = solver.trace()

    ks = hats_py.init_knowledge(g, actual_world)
    parts = hats_py.compute_partitions(g, ks.world_count)

    # Per-agent: worlds consistent with their actual observation (view-based partition class)
    agent_view_worlds = []
    for i in range(n):
        actual_view = ks.actual_views[i]
        cls = parts[i].classes.get(actual_view, [])
        agent_view_worlds.append(frozenset(cls))

    # Actual hat color of each agent: bit i of actual_world
    actual_hat = [(actual_world >> i) & 1 for i in range(n)]

    # All worlds tracked by the solver (world 0 excluded by convention)
    all_worlds = list(range(1, ks.world_count))
    world_labels = [format(w, f"0{n}b") for w in all_worlds]

    # Precompute cumulative guessed colors so frames are idempotent
    running_guessed = [-1] * n
    cumulative_guessed: list[list[int]] = []
    for rec in trace:
        for j, player in enumerate(rec.guessed_players):
            running_guessed[player] = rec.guessed_colors[j]
        cumulative_guessed.append(list(running_guessed))

    # Build NetworkX graph
    G = nx.Graph()
    G.add_nodes_from(range(n))
    for u in range(n):
        for v in range(u + 1, n):
            if g.has_edge(u, v):
                G.add_edge(u, v)

    pos = nx.circular_layout(G) if n <= 4 else nx.spring_layout(G, seed=42)

    fig, (ax_g, ax_m) = plt.subplots(1, 2, figsize=(14, max(5, n + 2)))
    hat_str = " ".join(f"A{i}={'R' if c else 'B'}" for i, c in enumerate(actual_hat))
    fig.suptitle(
        f"HATS DEL — {args.topology} n={n}  |  świat={format(actual_world, f'0{n}b')}  [{hat_str}]"
        "\n(R=czerwony, B=biały/niebieski)",
        fontsize=10,
    )

    def draw_frame(frame_idx: int):
        ax_g.clear()
        ax_m.clear()

        rec = trace[frame_idx]
        gc = cumulative_guessed[frame_idx]

        node_colors = []
        for player in range(n):
            if gc[player] == 1:
                node_colors.append("crimson")
            elif gc[player] == 0:
                node_colors.append("steelblue")
            else:
                node_colors.append("dimgray")

        nx.draw_networkx(
            G, pos, ax=ax_g,
            node_color=node_colors,
            node_size=900,
            labels={v: str(v) for v in range(n)},
            font_color="white",
            font_weight="bold",
            edge_color="gray",
        )
        ax_g.set_title(f"Graf agentów — runda {rec.round}", fontsize=10)
        ax_g.axis("off")

        # Matrix: rows = agents, cols = worlds
        # Cell (agent, world) lit iff world is globally valid AND consistent with agent's view
        valid_set = frozenset(rec.valid_worlds)
        mat = np.zeros((n, len(all_worlds)))
        for row in range(n):
            for col, w in enumerate(all_worlds):
                if w in valid_set and w in agent_view_worlds[row]:
                    mat[row, col] = 1.0

        img = ax_m.imshow(mat, aspect="auto", cmap="Blues", vmin=0, vmax=1,
                          interpolation="nearest")

        ax_m.set_xticks(range(len(all_worlds)))
        ax_m.set_xticklabels(world_labels, rotation=90, fontsize=7)
        ax_m.set_yticks(range(n))
        ax_m.set_yticklabels([f"Agent {i}" for i in range(n)], fontsize=9)
        ax_m.set_xlabel("Świat (bitmaska kapeluszy)", fontsize=9)

        # Minor grid between cells
        ax_m.set_xticks(np.arange(-0.5, len(all_worlds), 1), minor=True)
        ax_m.set_yticks(np.arange(-0.5, n, 1), minor=True)
        ax_m.grid(which="minor", color="white", linewidth=0.8)
        ax_m.tick_params(which="minor", bottom=False, left=False)

        if rec.silence:
            subtitle = f"Runda {rec.round}  (cisza — eliminacja sprzecznych światów)"
        elif rec.guessed_players:
            who = ", ".join(f"agent {p}" for p in rec.guessed_players)
            subtitle = f"Runda {rec.round}  (odgaduje: {who})"
        else:
            subtitle = f"Runda {rec.round}"
        ax_m.set_title(subtitle, fontsize=10)

        return [img]

    draw_frame(0)
    plt.tight_layout()

    anim = animation.FuncAnimation(
        fig, draw_frame, frames=len(trace), interval=1500, blit=False, repeat=True,
    )

    if args.save:
        anim.save("hats_simulation.gif", writer=animation.PillowWriter(fps=1))
        print(f"Zapisano do hats_simulation.gif  ({len(trace)} klatek)")
    else:
        plt.show()


if __name__ == "__main__":
    main()
