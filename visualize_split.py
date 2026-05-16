#!/usr/bin/env python3
import math
import sys
from pathlib import Path
import numpy as np

sys.path.insert(0, str(Path(__file__).parent / "build"))

import hats_py
import networkx as nx
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import matplotlib.gridspec as gridspec

class PyRoundRecord:
    def __init__(self, round_num, guessed_players, guessed_colors, valid_worlds, silence):
        self.round = round_num
        self.guessed_players = guessed_players
        self.guessed_colors = guessed_colors
        self.valid_worlds = valid_worlds
        self.silence = silence

def py_trace(g, ks, actual_world, parts, n):
    valid_worlds = set(range(1, ks.world_count))
    already_guessed = [False] * n
    trace = []
    rounds = 0
    total_guess = 0

    while True:
        guessers = []
        for player in range(n):
            if already_guessed[player]:
                continue
            
            actual_view = ks.actual_views[player]
            cls = parts[player].classes.get(actual_view, [])
            
            seen_zero = False
            seen_one = False
            for w in cls:
                if w in valid_worlds:
                    color = (w >> player) & 1
                    if color == 1:
                        seen_one = True
                    else:
                        seen_zero = True
            
            if seen_one != seen_zero and (seen_one or seen_zero):
                decided_color = 1 if seen_one else 0
                guessers.append((player, decided_color))
        
        guessed_players = [p for p, _ in guessers]
        guessed_colors = [c for _, c in guessers]
        
        trace.append(PyRoundRecord(rounds, guessed_players, guessed_colors, list(valid_worlds), silence=False))
        
        for p, _ in guessers:
            already_guessed[p] = True
            total_guess += 1
            
        if total_guess == n:
            break
            
        if guessers:
            for gp, gc in guessers:
                to_remove = set()
                for w in valid_worlds:
                    if ((w >> gp) & 1) != gc:
                        to_remove.add(w)
                        continue
                    
                    view_w = w & parts[gp].neighbors_mask
                    cls_w = parts[gp].classes.get(view_w, [])
                    sz, so = False, False
                    for w2 in cls_w:
                        if w2 in valid_worlds:
                            if ((w2 >> gp) & 1): so = True
                            else: sz = True
                    if sz == so:
                        to_remove.add(w)
                valid_worlds -= to_remove
        else:
            to_remove = set()
            for player in range(n):
                if already_guessed[player]: continue
                for view, cls in parts[player].classes.items():
                    sz, so = False, False
                    for w in cls:
                        if w in valid_worlds:
                            if ((w >> player) & 1): so = True
                            else: sz = True
                    if sz != so and (sz or so):
                        for w in cls:
                            if w in valid_worlds:
                                to_remove.add(w)
            
            if not to_remove:
                break
            valid_worlds -= to_remove
            trace.append(PyRoundRecord(rounds, [], [], list(valid_worlds), silence=True))
            
        rounds += 1
        
    return trace

def run_simulation(topology: str, n: int, actual_world: int, save_anim: bool):
    print(f"\n--- Uruchamianie symulacji: Topologia={topology.capitalize()}, n={n}, World State={format(actual_world, f'0{n}b')} ---")
    if topology == "complete":
        g = hats_py.Graph.make_complete(n)
    elif topology == "cycle":
        g = hats_py.Graph.make_cycle(n)
    elif topology == "star":
        g = hats_py.Graph.make_star(n)
    elif topology == "path":
        g = hats_py.Graph(n)
        for i in range(n - 1):
            g.add_edge(i, i + 1)
    elif topology == "wheel":
        g = hats_py.Graph(n)
        for i in range(n - 1):
            g.add_edge(i, (i + 1) % (n - 1))
            g.add_edge(i, n - 1)
    elif topology == "house":
        g = hats_py.Graph(n)
        g.add_edge(0, 1)
        g.add_edge(1, 2)
        g.add_edge(2, 3)
        g.add_edge(3, 0)
        g.add_edge(0, 4)
        g.add_edge(1, 4)
    else:
        raise ValueError(f"Nieznana topologia: {topology}")
        
    ks = hats_py.init_knowledge(g, actual_world)
    parts = hats_py.compute_partitions(g, ks.world_count)

    trace = py_trace(g, ks, actual_world, parts, n)

    actual_hat = [(actual_world >> i) & 1 for i in range(n)]
    all_worlds = list(range(1, ks.world_count))
    world_labels = [format(w, f"0{n}b") for w in all_worlds]

    agent_view_worlds = []
    for i in range(n):
        actual_view = ks.actual_views[i]
        cls = parts[i].classes.get(actual_view, [])
        agent_view_worlds.append(frozenset(cls))

    running_guessed = [-1] * n
    cumulative_guessed = []
    for rec in trace:
        for j, player in enumerate(rec.guessed_players):
            running_guessed[player] = rec.guessed_colors[j]
        cumulative_guessed.append(list(running_guessed))

    G = nx.Graph()
    G.add_nodes_from(range(n))
    for u in range(n):
        for v in range(u + 1, n):
            if g.has_edge(u, v):
                G.add_edge(u, v)

    pos = nx.circular_layout(G)

    plt.style.use('seaborn-v0_8-darkgrid')
    fig = plt.figure(figsize=(16, 9))
    fig.subplots_adjust(top=0.90)
    
    graph_cols = 2
    graph_rows = math.ceil(n / graph_cols)
    
    gs = gridspec.GridSpec(graph_rows, graph_cols + 1, figure=fig, wspace=0.3, hspace=0.3)
    
    axs_graphs = []
    for i in range(n):
        r = i // graph_cols
        c = i % graph_cols
        axs_graphs.append(fig.add_subplot(gs[r, c]))
        
    ax_heatmap = fig.add_subplot(gs[:, graph_cols:])

    hat_str = " | ".join(f"P{i}: {'Red' if c else 'Blue'}" for i, c in enumerate(actual_hat))
    fig.suptitle(f"Dynamic Epistemic Logic: Hat Guessing Game\nTopology: {topology.capitalize()} (n={n})   |   World State: {format(actual_world, f'0{n}b')}   [{hat_str}]", 
                 fontsize=14, fontweight='bold', y=0.98)

    def draw_frame(frame_idx: int):
        rec = trace[frame_idx]
        gc = cumulative_guessed[frame_idx]
        valid_set = frozenset(rec.valid_worlds)

        ax_heatmap.clear()
        mat = np.zeros((n, len(all_worlds)))
        for row in range(n):
            for col, w in enumerate(all_worlds):
                if w in valid_set and w in agent_view_worlds[row]:
                    mat[row, col] = 1.0

        ax_heatmap.imshow(mat, aspect="auto", cmap="Blues", vmin=0, vmax=1, interpolation="nearest")
        ax_heatmap.set_xticks(range(len(all_worlds)))
        ax_heatmap.set_xticklabels(world_labels, rotation=90, fontsize=9, fontfamily='monospace')
        ax_heatmap.set_yticks(range(n))
        ax_heatmap.set_yticklabels([f"Agent {i}" for i in range(n)], fontsize=11, fontweight='bold')
        ax_heatmap.set_xlabel("Hypothetical Worlds (Binary Mask)", fontsize=12, labelpad=10)

        ax_heatmap.set_xticks(np.arange(-0.5, len(all_worlds), 1), minor=True)
        ax_heatmap.set_yticks(np.arange(-0.5, n, 1), minor=True)
        ax_heatmap.grid(which="minor", color="white", linewidth=1.2)
        ax_heatmap.tick_params(which="minor", bottom=False, left=False)

        if rec.silence:
            title_hm = f"Round {rec.round} — Silence (Contradictions Eliminated)"
            hm_color = "#4682B4"
        elif rec.guessed_players:
            title_hm = f"Round {rec.round} — Public Announcement by: {', '.join(map(lambda x: 'P'+str(x), rec.guessed_players))}"
            hm_color = "#2E8B57"
        else:
            if frame_idx == len(trace) - 1:
                title_hm = f"Deadlock! (Indefinite Silence)"
                hm_color = "#D32F2F"
            else:
                title_hm = f"Round {rec.round} — Initial State"
                hm_color = "#696969"
            
        ax_heatmap.set_title(title_hm, fontsize=13, fontweight='bold', color=hm_color, pad=15)
        ax_heatmap.spines['top'].set_visible(False)
        ax_heatmap.spines['right'].set_visible(False)

        for perspective_agent in range(n):
            ax = axs_graphs[perspective_agent]
            ax.clear()

            node_colors = []
            for target_node in range(n):
                if target_node == perspective_agent:
                    if gc[perspective_agent] == 1:
                        node_colors.append("#D32F2F") 
                    elif gc[perspective_agent] == 0:
                        node_colors.append("#1976D2") 
                    else:
                        node_colors.append("#E0E0E0") 
                else:
                    has_edge = g.has_edge(perspective_agent, target_node) 
                    has_guessed_globally = (gc[target_node] != -1)
                    
                    if has_edge or has_guessed_globally:
                        if actual_hat[target_node] == 1:
                            node_colors.append("#D32F2F")
                        else:
                            node_colors.append("#1976D2")
                    else:
                        node_colors.append("#E0E0E0") 
            
            if perspective_agent in rec.guessed_players:
                frame_agent_title = f"Agent {perspective_agent}'s Perspective\n✓ Guessed Hat Color!"
                ax.set_facecolor('#F0FAEF') 
            else:
                frame_agent_title = f"Agent {perspective_agent}'s Perspective"
                ax.set_facecolor('#FFFFFF')

            nx.draw_networkx(
                G, pos, ax=ax,
                node_color=node_colors,
                node_size=1600,
                labels={v: f"P{v}" for v in range(n)},
                font_color="black",
                font_weight="bold",
                font_size=10,
                edge_color="#757575",
                width=1.5
            )
            ax.set_title(frame_agent_title, fontsize=11, fontweight='bold', pad=10)
            
            nx.draw_networkx_nodes(G, pos, ax=ax, nodelist=[perspective_agent], 
                                   node_size=2000, edgecolors='#FF9800', linewidths=3.5, node_color=node_colors[perspective_agent])
            
            ax.axis("off")

        return axs_graphs + [ax_heatmap]

    draw_frame(0)

    anim = animation.FuncAnimation(fig, draw_frame, frames=len(trace), interval=2200, blit=False, repeat=False)

    if save_anim:
        filename = f"hats_del_{topology}_n{n}.gif"
        print(f"Generating animation frames... Saving as {filename}")
        anim.save(filename, writer=animation.PillowWriter(fps=0.6))
        print(f"Successfully saved {filename} ({len(trace)} frames)")
    else:
        plt.show()
    
    plt.close(fig)

def main():
    save_anim = True
    
    # run_simulation("complete", 4, 0b0110, save_anim)
    # run_simulation("cycle", 4, 0b0001, save_anim)
    # run_simulation("star", 4, 0b0110, save_anim)
    # run_simulation("cycle", 3, 0b010, save_anim)
    # run_simulation("cycle", 5, 0b00001, save_anim)
    run_simulation("path", 4, 0b0011, save_anim)
    run_simulation("wheel", 5, 0b10000, save_anim)
    run_simulation("house", 5, 0b00001, save_anim)
    
if __name__ == "__main__":
    main()