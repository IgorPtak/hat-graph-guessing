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
    print(f"\n--- Uruchamianie symulacji: Topologia={topology.capitalize()}, n={n}, Stan={format(actual_world, f'0{n}b')} ---")
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
    elif topology == "kite":
        g = hats_py.Graph(n) # n=4
        # Trójkąt (0, 1, 2)
        g.add_edge(0, 1)
        g.add_edge(1, 2)
        g.add_edge(2, 0)
        # Ogon (3 podpięte pod 2)
        g.add_edge(2, 3)
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

    if topology == "kite":
        pos = {0: (0, 0), 1: (0, 2), 2: (2, 1), 3: (4, 1)}  # Klasyczny latawiec bokiem
    elif topology == "house":
        pos = {0: (0, 0), 1: (2, 0), 2: (2, -2), 3: (0, -2), 4: (1, 1.5)} # Domek
    elif topology == "path":
        pos = {i: (i, 0) for i in range(n)} # Ścieżka poziomo
    elif topology == "wheel":
        # Środek to ostatni węzeł (n-1). Dla n=5 daje nam to wierzchołek 4 na {0,0}.
        # Pozostałe (0,1,2,3) w równych odstępach koła, dla estetyki ułożone na osiach
        pos = {4: (0, 0), 0: (0, 2), 1: (2, 0), 2: (0, -2), 3: (-2, 0)}
    else:
        # domyślnie koło
        pos = nx.circular_layout(G)

        plt.style.use('seaborn-v0_8-white')
    # Podnosimy rozmiar figury o +2 jednostki (z 16x9 na 18x10), co da dużo oddechu
    fig = plt.figure(figsize=(18, 10), facecolor='#FAFAFA')
    # Podnosimy bottom z 0.08 do 0.12, żeby podpisy pionowe spokojnie weszły bez ucinania
    fig.subplots_adjust(top=0.88, bottom=0.12, left=0.03, right=0.97)
    
    graph_cols = 2
    graph_rows = math.ceil(n / graph_cols)
    
    gs = gridspec.GridSpec(graph_rows, graph_cols + 1, figure=fig, wspace=0.15, hspace=0.35)
    
    axs_graphs = []
    for i in range(n):
        r = i // graph_cols
        c = i % graph_cols
        axs_graphs.append(fig.add_subplot(gs[r, c]))
        
    ax_heatmap = fig.add_subplot(gs[:, graph_cols:])

    hat_str = " | ".join(f"G{i}: {'Czerwony' if c else 'Niebieski'}" for i, c in enumerate(actual_hat))
    fig.suptitle(f"Dynamiczna Logika Epistemiczna: Problem Kapeluszy\nTopologia: {topology.capitalize()} (n={n})   |   Stan Rzeczywisty: {format(actual_world, f'0{n}b')}   [{hat_str}]", 
                 fontsize=15, fontweight='normal', y=0.97, color='#2C3E50')

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

        # Zastosowanie jasnej, estetycznej mapy kolorów
        ax_heatmap.imshow(mat, aspect="auto", cmap="YlGnBu", vmin=0, vmax=1, interpolation="nearest")
        ax_heatmap.set_xticks(range(len(all_worlds)))
        ax_heatmap.set_xticklabels(world_labels, rotation=90, fontsize=8, fontfamily='monospace', color='#555555')
        ax_heatmap.set_yticks(range(n))
        ax_heatmap.set_yticklabels([f"Gracz {i}" for i in range(n)], fontsize=11, fontweight='normal', color='#333333')
        ax_heatmap.set_xlabel("Hipotetyczne Światy (Maska Binarna)", fontsize=11, labelpad=10, color='#555555')

        ax_heatmap.set_xticks(np.arange(-0.5, len(all_worlds), 1), minor=True)
        ax_heatmap.set_yticks(np.arange(-0.5, n, 1), minor=True)
        ax_heatmap.grid(which="minor", color="#FFFFFF", linewidth=1.5)
        ax_heatmap.tick_params(which="minor", bottom=False, left=False)
        ax_heatmap.tick_params(axis='both', which='major', labelsize=10, colors='#555555')

        # Minimalistyczne tytuły
        if rec.silence:
            title_hm = f"Runda {rec.round} — Cisza (Eliminacja Sprzeczności)"
            hm_color = "#34495E" # Ciemny granat
        elif rec.guessed_players:
            title_hm = f"Runda {rec.round} — Deklaracja Graczy: {', '.join(map(lambda x: 'G'+str(x), rec.guessed_players))}"
            hm_color = "#27AE60" # Estetyczna zieleń
        else:
            if frame_idx == len(trace) - 1:
                title_hm = f"Martwy Punkt (Brak Wystarczających Informacji)"
                hm_color = "#E74C3C" # Stonowana czerwień
            else:
                title_hm = f"Runda {rec.round} — Stan Początkowy"
                hm_color = "#7F8C8D" # Chłodny szary
            
        ax_heatmap.set_title(title_hm, fontsize=12, fontweight='bold', color=hm_color, pad=15)
        ax_heatmap.spines['top'].set_visible(False)
        ax_heatmap.spines['right'].set_visible(False)
        ax_heatmap.spines['bottom'].set_color('#E0E0E0')
        ax_heatmap.spines['left'].set_color('#E0E0E0')

        for perspective_agent in range(n):
            ax = axs_graphs[perspective_agent]
            ax.clear()

            node_colors = []
            for target_node in range(n):
                if target_node == perspective_agent:
                    if gc[perspective_agent] == 1:
                        node_colors.append("#E74C3C") # Prezentacyjna czerwień
                    elif gc[perspective_agent] == 0:
                        node_colors.append("#2980B9") # Prezentacyjny niebieski
                    else:
                        node_colors.append("#ECF0F1") # Jasnoszary (Brak wiedzy)
                else:
                    has_edge = g.has_edge(perspective_agent, target_node) 
                    has_guessed_globally = (gc[target_node] != -1)
                    
                    if has_edge or has_guessed_globally:
                        if actual_hat[target_node] == 1:
                            node_colors.append("#E74C3C")
                        else:
                            node_colors.append("#2980B9")
                    else:
                        node_colors.append("#ECF0F1") 
            
            if perspective_agent in rec.guessed_players:
                frame_agent_title = f"Perspektywa Gracza {perspective_agent}\n(Zgadł kolor!)"
                ax.set_facecolor('#F8FFF8') # Niemal niedostrzegalna zieleń, profesjonalna
            else:
                frame_agent_title = f"Perspektywa Gracza {perspective_agent}"
                ax.set_facecolor('#FAFAFA')

            nx.draw_networkx(
                G, pos, ax=ax,
                node_color=node_colors,
                node_size=1200,
                labels={v: f"G{v}" for v in range(n)},
                font_color="#2C3E50",
                font_weight="bold",
                font_size=9,
                edge_color="#BDC3C7",
                width=1.5
            )
            ax.set_title(frame_agent_title, fontsize=10, fontweight='normal', color="#34495E", pad=8)
            
            # Subtelne obramowanie wokół głównego agenta
            nx.draw_networkx_nodes(G, pos, ax=ax, nodelist=[perspective_agent], 
                                   node_size=1500, edgecolors='#F39C12', linewidths=2.5, node_color=node_colors[perspective_agent])
            
            ax.axis("off")
            
            # DODATKOWY PADDING (margines wokół grafu węzłów)
            margin = 0.5
            x_values = [p[0] for p in pos.values()]
            y_values = [p[1] for p in pos.values()]
            ax.set_xlim(min(x_values) - margin, max(x_values) + margin)
            ax.set_ylim(min(y_values) - margin, max(y_values) + margin)

        return axs_graphs + [ax_heatmap]

    draw_frame(0)

    anim = animation.FuncAnimation(fig, draw_frame, frames=len(trace), interval=2500, blit=False, repeat=False)

    if save_anim:
        filename = f"hats_del_{topology}_n{n}.gif"
        print(f"Generowanie animacji... Zapisywanie jako {filename}")
        anim.save(filename, writer=animation.PillowWriter(fps=0.5)) # Nieco wolniejsze dla prezentacji (0.5 fps)
        print(f"Zapisano pomyślnie {filename} ({len(trace)} klatek)")
    else:
        plt.show()
    
    plt.close(fig)
    
def main():
    save_anim = True
    
    run_simulation("complete", 4, 0b0110, save_anim)
    run_simulation("complete", 5, 0b01111, save_anim)
    run_simulation("cycle", 4, 0b0001, save_anim)
    run_simulation("star", 4, 0b0110, save_anim)
    run_simulation("path", 4, 0b0011, save_anim)
    run_simulation("wheel", 5, 0b10000, save_anim)
    
    # House asymetria podwójna:
    run_simulation("house", 5, 0b10001, save_anim)
    
    # Kite asymetria centralna (G2 ma jedynkę, reszta G0,G1,G3 puste)
    run_simulation("kite", 4, 0b0100, save_anim)
    
if __name__ == "__main__":
    main()