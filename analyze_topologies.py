#!/usr/bin/env python3
"""
Szuka ciekawych topologii grafów, które nie kończą się zakleszczeniem,
i generuje wykresy do raportu.
"""
import sys
import math
import itertools
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent / "build"))
import hats_py
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import networkx as nx
import numpy as np

# ─── helpers ──────────────────────────────────────────────────────────────────

def make_wheel(n):
    """Wheel W_n: n-1 leaves (0..n-2) + center (n-1) with a leaf-cycle."""
    g = hats_py.Graph(n)
    for i in range(n - 1):
        g.add_edge(i, (i + 1) % (n - 1))
        g.add_edge(i, n - 1)
    return g

def make_kn_minus_edge(n, u, v):
    """K_n with edge {u,v} removed."""
    g = hats_py.Graph.make_complete(n)
    # We can't remove; rebuild without it.
    g2 = hats_py.Graph(n)
    for a in range(n):
        for b in range(a+1, n):
            if not ((a == u and b == v) or (a == v and b == u)):
                g2.add_edge(a, b)
    return g2

def make_complete_tripartite(k):
    """K_{k,k,k}: three groups of k nodes, all cross-group edges."""
    n = 3 * k
    g = hats_py.Graph(n)
    groups = [list(range(i*k, (i+1)*k)) for i in range(3)]
    for gi, gj in itertools.combinations(range(3), 2):
        for u in groups[gi]:
            for v in groups[gj]:
                g.add_edge(u, v)
    return g

def make_complete_bipartite(m, r):
    """K_{m,r}: left={0..m-1}, right={m..m+r-1}."""
    n = m + r
    g = hats_py.Graph(n)
    for u in range(m):
        for v in range(m, n):
            g.add_edge(u, v)
    return g

def make_friendship(k):
    """Friendship graph F_k: k triangles sharing center node (last node)."""
    n = 2*k + 1
    center = n - 1
    g = hats_py.Graph(n)
    for i in range(k):
        a, b = 2*i, 2*i+1
        g.add_edge(a, b)
        g.add_edge(a, center)
        g.add_edge(b, center)
    return g

def make_cocktail_party(k):
    """Cocktail party graph K_{k×2}: K_{2k} minus a perfect matching."""
    n = 2 * k
    g = hats_py.Graph(n)
    for a in range(n):
        for b in range(a+1, n):
            # skip edges (0,1),(2,3),(4,5),... (pairs)
            if not (a == 2*(a//2) and b == a+1):
                g.add_edge(a, b)
    return g

def exhaust(g, n):
    """Test all worlds 1..2^n-1, return (successes, deadlocks, results)."""
    successes = []
    deadlocks = []
    for w in range(1, 1 << n):
        s = hats_py.Solver(g, w)
        r = s.run()
        if r.success:
            successes.append((w, r.rounds))
        else:
            deadlocks.append(w)
    return successes, deadlocks

def entropy_trace(g, n, w):
    """Return (events, valid_counts, H_matrix) for a given world."""
    s = hats_py.Solver(g, w)
    trace = s.trace()
    events = []
    valid_counts = []
    H_matrix = []  # H_matrix[round][agent]
    for rec in trace:
        if rec.silence:
            events.append("CISZA")
        elif rec.guessed_players:
            events.append("ODGADUJE: " + ",".join(str(p) for p in rec.guessed_players))
        else:
            events.append("INIT")
        valid_counts.append(len(rec.valid_worlds))
        H_row = [math.log2(sz) if sz > 0 else 0.0 for sz in rec.agent_class_sizes]
        H_matrix.append(H_row)
    return events, valid_counts, H_matrix

# ─── analysis ──────────────────────────────────────────────────────────────────

print("=" * 60)
print("ANALIZA TOPOLOGII — szukanie ciekawych grafów")
print("=" * 60)

results_summary = {}  # name -> (n, num_success, num_deadlock, total)

# 1. Wheel graphs
print("\n--- Koło W_n ---")
for n in [4, 5, 6, 7]:
    g = make_wheel(n)
    succ, dead = exhaust(g, n)
    total = (1 << n) - 1
    print(f"  W_{n}: n={n}, sukces={len(succ)}/{total}  succ_worlds={[w for w,_ in succ]}")
    results_summary[f"W_{n}"] = (n, len(succ), len(dead), total)

# 2. K_n minus one edge
print("\n--- K_n minus krawędź {n-2, n-1} ---")
for n in [4, 5, 6]:
    g = make_kn_minus_edge(n, n-2, n-1)
    succ, dead = exhaust(g, n)
    total = (1 << n) - 1
    print(f"  K_{n}\\e: n={n}, sukces={len(succ)}/{total}")
    results_summary[f"K{n}\\e"] = (n, len(succ), len(dead), total)

# 3. Complete tripartite K_{2,2,2}
print("\n--- Trójdzielny pełny K_{{2,2,2}} (n=6) ---")
g_t = make_complete_tripartite(2)
succ, dead = exhaust(g_t, 6)
total = (1 << 6) - 1
print(f"  K_{{2,2,2}}: n=6, sukces={len(succ)}/{total}")
results_summary["K_{2,2,2}"] = (6, len(succ), len(dead), total)

# 4. Complete bipartite K_{m,n}
print("\n--- Dwudzielny pełny K_{{m,r}} ---")
for m, r in [(2,2), (2,3), (3,3)]:
    n = m + r
    g = make_complete_bipartite(m, r)
    succ, dead = exhaust(g, n)
    total = (1 << n) - 1
    print(f"  K_{{{m},{r}}}: n={n}, sukces={len(succ)}/{total}")
    results_summary[f"K_{{{m},{r}}}"] = (n, len(succ), len(dead), total)

# 5. Friendship graph
print("\n--- Graf przyjaźni F_k ---")
for k in [2, 3]:
    n = 2*k + 1
    g = make_friendship(k)
    succ, dead = exhaust(g, n)
    total = (1 << n) - 1
    print(f"  F_{k}: n={n}, sukces={len(succ)}/{total}  succ_worlds={[w for w,_ in succ]}")
    results_summary[f"F_{k}"] = (n, len(succ), len(dead), total)

# 6. Cocktail party graph K_{k×2}
print("\n--- Graf koktajlowy CP_k ---")
for k in [3, 4]:
    n = 2*k
    g = make_cocktail_party(k)
    succ, dead = exhaust(g, n)
    total = (1 << n) - 1
    print(f"  CP_{k}: n={n}, sukces={len(succ)}/{total}")
    results_summary[f"CP_{k}"] = (n, len(succ), len(dead), total)

# 7. Cycle with two chords (more symmetric breaking)
print("\n--- C_6 z dwoma cięciwami (0,3) i (1,4) ---")
g_c6 = hats_py.Graph.make_cycle(6)
g_c6.add_edge(0, 3)
g_c6.add_edge(1, 4)
succ, dead = exhaust(g_c6, 6)
total = (1 << 6) - 1
print(f"  C_6+2chords: n=6, sukces={len(succ)}/{total}  succ_worlds={[w for w,_ in succ]}")
results_summary["C_6+2chords"] = (6, len(succ), len(dead), total)

# 8. C_5 with one chord
print("\n--- C_5 z cięciwą (0,2) ---")
g_c5 = hats_py.Graph.make_cycle(5)
g_c5.add_edge(0, 2)
succ, dead = exhaust(g_c5, 5)
total = (1 << 5) - 1
print(f"  C_5+(0,2): n=5, sukces={len(succ)}/{total}  succ_worlds={[w for w,_ in succ]}")
results_summary["C_5+(0,2)"] = (5, len(succ), len(dead), total)

# 9. C_5 with two chords (makes it like W_5 but different)
print("\n--- C_5 z cięciwami (0,2) i (0,3) ---")
g_c5b = hats_py.Graph.make_cycle(5)
g_c5b.add_edge(0, 2)
g_c5b.add_edge(0, 3)
succ, dead = exhaust(g_c5b, 5)
total = (1 << 5) - 1
print(f"  C_5+(0,2)+(0,3): n=5, sukces={len(succ)}/{total}  succ_worlds={[w for w,_ in succ]}")
results_summary["C_5+2ch"] = (5, len(succ), len(dead), total)

# 10. K_{2,2,2} vs K_6 comparison
print("\n--- K_6 (odniesienie) ---")
g_k6 = hats_py.Graph.make_complete(6)
succ, dead = exhaust(g_k6, 6)
total = (1 << 6) - 1
print(f"  K_6: n=6, sukces={len(succ)}/{total}")
results_summary["K_6"] = (6, len(succ), len(dead), total)

print("\n" + "=" * 60)
print("PODSUMOWANIE:")
print(f"{'Graf':<18} {'n':>3} {'Sukces':>8} {'Razem':>8} {'%':>7}")
print("-" * 50)
for name, (n, ns, nd, tot) in sorted(results_summary.items(), key=lambda x: -x[1][1]/x[1][3]):
    pct = 100 * ns / tot
    print(f"{name:<18} {n:>3} {ns:>8} {tot:>8} {pct:>6.1f}%")

# ─── generate entropy plots ────────────────────────────────────────────────────

print("\n--- Generowanie wykresów entropii ---")

def plot_entropy(g, n, w, title, filename):
    """Generate entropy evolution plot."""
    events, valid_counts, H_matrix = entropy_trace(g, n, w)
    rounds = list(range(len(events)))

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(8, 5), gridspec_kw={'height_ratios': [2, 1]})

    colors = plt.cm.tab10(np.linspace(0, 1, n))
    for i in range(n):
        H_vals = [H_matrix[t][i] for t in range(len(events))]
        ax1.plot(rounds, H_vals, 'o-', color=colors[i], label=f"Agent {i}", linewidth=2, markersize=5)

    ax1.set_ylabel("Entropia epistemiczna $H_i(t)$ [bity]", fontsize=11)
    ax1.set_title(title, fontsize=12, fontweight='bold')
    ax1.legend(loc='upper right', fontsize=8, ncol=min(n, 4))
    ax1.grid(True, alpha=0.3)
    ax1.set_xticks(rounds)

    # Color background by event type
    for t, ev in enumerate(events):
        if "CISZA" in ev:
            ax1.axvspan(t - 0.4, t + 0.4, alpha=0.1, color='orange')
        elif "ODGADUJE" in ev:
            ax1.axvspan(t - 0.4, t + 0.4, alpha=0.1, color='green')

    ax2.bar(rounds, valid_counts, color=['#4c72b0' if 'CISZA' in e else '#dd8452' if 'ODGADUJE' in e else '#55a868' for e in events])
    ax2.set_xlabel("Runda", fontsize=11)
    ax2.set_ylabel("$|W_{\\rm valid}|$", fontsize=11)
    ax2.set_xticks(rounds)
    ax2.set_xticklabels([f"R{t}\n{ev[:10]}" for t, ev in enumerate(events)], fontsize=7)
    ax2.grid(True, alpha=0.3, axis='y')

    plt.tight_layout()
    plt.savefig(filename, dpi=150, bbox_inches='tight')
    plt.close()
    print(f"  Zapisano: {filename}")

# K_n minus edge (n=5): interesting dynamics
g_k5e = make_kn_minus_edge(5, 3, 4)
succ5, dead5 = exhaust(g_k5e, 5)
if succ5:
    # Pick a world with k=2 (middling case)
    for w, rounds_w in succ5:
        if bin(w).count('1') == 2:
            plot_entropy(g_k5e, 5, w,
                f"$K_5 \\setminus e$, $w = {format(w, '05b')}$ (sukces, {rounds_w} rund)",
                "fig_entropy_k5e.png")
            break

# Cocktail party CP_3 (n=6, K_{3x2}): K_6 minus perfect matching
g_cp3 = make_cocktail_party(3)
succ_cp3, dead_cp3 = exhaust(g_cp3, 6)
print(f"\nCP_3 (n=6): sukces={len(succ_cp3)}/63, przykłady=[{[w for w,_ in succ_cp3[:5]]}...]")
if succ_cp3:
    # Pick a world with k=3
    for w, rounds_w in succ_cp3:
        if bin(w).count('1') == 3:
            plot_entropy(g_cp3, 6, w,
                f"Graf koktajlowy $CP_3$ ($n=6$), $w={format(w, '06b')}$ (sukces)",
                "fig_entropy_cp3.png")
            break

# Friendship graph F_2 (n=5): analyze
g_f2 = make_friendship(2)
succ_f2, dead_f2 = exhaust(g_f2, 5)
print(f"\nF_2 (n=5): sukces={len(succ_f2)}/31, succ_worlds={[w for w,_ in succ_f2]}")
if succ_f2:
    w_ex, rnd_ex = succ_f2[0]
    plot_entropy(g_f2, 5, w_ex,
        f"Graf przyjaźni $F_2$ ($n=5$), $w={format(w_ex, '05b')}$ (sukces)",
        "fig_entropy_f2.png")

# ─── bar chart: success rates ──────────────────────────────────────────────────

print("\n--- Generowanie wykresu słupkowego ---")

# Curated set for the bar chart
bar_data = {
    "K_3": (3, 7, 7),
    "K_4": (4, 15, 15),
    "K_5": (5, 31, 31),
    "S_4": (4, 1, 15),
    "S_5": (5, 1, 31),
    "C_4": (4, 0, 15),
    "C_5": (5, 0, 31),
    "P_4": (4, 0, 15),
    "W_5": (5, None, 31),
    "K_5\\e": (5, None, 31),
    "F_2": (5, None, 31),
    "CP_3": (6, None, 63),
}

# Fill in computed values
g_k3 = hats_py.Graph.make_complete(3)
g_k4 = hats_py.Graph.make_complete(4)
g_k5 = hats_py.Graph.make_complete(5)
g_s4 = hats_py.Graph.make_star(4)
g_s5 = hats_py.Graph.make_star(5)
g_c4 = hats_py.Graph.make_cycle(4)
g_c5 = hats_py.Graph.make_cycle(5)
g_p4 = hats_py.Graph.make_path(4)
g_w5 = make_wheel(5)
g_k5e_bar = make_kn_minus_edge(5, 3, 4)

computed = {
    "W_5":   len(exhaust(g_w5, 5)[0]),
    "K_5\\e": len(exhaust(g_k5e_bar, 5)[0]),
    "F_2":   len(succ_f2),
    "CP_3":  len(succ_cp3),
}

curated = [
    ("$K_3$",     7,  7),
    ("$K_4$",     15, 15),
    ("$K_5$",     31, 31),
    ("$S_4$",     1,  15),
    ("$S_5$",     1,  31),
    ("$C_4$",     0,  15),
    ("$C_5$",     0,  31),
    ("$P_4$",     0,  15),
    ("$W_5$",     computed["W_5"],    31),
    ("$K_5\\!\\setminus\\!e$", computed["K_5\\e"], 31),
    ("$F_2$",     computed["F_2"],    31),
    ("$CP_3$",    computed["CP_3"],   63),
]

labels = [x[0] for x in curated]
pcts   = [100 * x[1] / x[2] for x in curated]
ns_abs = [x[1] for x in curated]
totals = [x[2] for x in curated]

fig, ax = plt.subplots(figsize=(11, 5))
bar_colors = ['#2ecc71' if p == 100 else '#e74c3c' if p == 0 else '#3498db' for p in pcts]
bars = ax.bar(labels, pcts, color=bar_colors, edgecolor='white', linewidth=0.8)

for bar, ns, tot in zip(bars, ns_abs, totals):
    ax.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 1.5,
            f"{ns}/{tot}", ha='center', va='bottom', fontsize=9, fontweight='bold')

ax.set_ylabel("Odsetek światów kończących się sukcesem [%]", fontsize=11)
ax.set_title("Porównanie topologii — skuteczność protokołu DEL", fontsize=13, fontweight='bold')
ax.set_ylim(0, 115)
ax.set_yticks(range(0, 101, 20))
ax.grid(True, alpha=0.3, axis='y')
ax.tick_params(axis='x', labelsize=10)

green_p = mpatches.Patch(color='#2ecc71', label='Zawsze sukces')
blue_p  = mpatches.Patch(color='#3498db', label='Częściowy sukces')
red_p   = mpatches.Patch(color='#e74c3c', label='Zawsze zakleszczenie')
ax.legend(handles=[green_p, blue_p, red_p], fontsize=10, loc='upper right')

plt.xticks(rotation=20, ha='right')
plt.tight_layout()
plt.savefig("fig_success_rates.png", dpi=150, bbox_inches='tight')
plt.close()
print("  Zapisano: fig_success_rates.png")

# ─── detailed analysis of K_n minus edge ──────────────────────────────────────

print("\n--- Szczegółowa analiza K_5\\e ---")
g_k5e2 = make_kn_minus_edge(5, 3, 4)
succ, dead = exhaust(g_k5e2, 5)
print(f"Sukces: {len(succ)}/31")
print(f"  succ_worlds: {[w for w,_ in succ]}")
for k in range(1, 6):
    s_k = [(w,r) for w,r in succ if bin(w).count('1') == k]
    d_k = [w for w in dead if bin(w).count('1') == k]
    total_k = sum(1 for w in range(1, 32) if bin(w).count('1') == k)
    print(f"  k={k}: sukces={len(s_k)}/{total_k}, rundy={set(r for _,r in s_k)}")

print("\n--- Szczegółowa analiza grafu koktajlowego CP_3 ---")
print(f"Sukces: {len(succ_cp3)}/63")
for k in range(1, 7):
    s_k = [(w,r) for w,r in succ_cp3 if bin(w).count('1') == k]
    total_k = sum(1 for w in range(1, 64) if bin(w).count('1') == k)
    print(f"  k={k}: sukces={len(s_k)}/{total_k}, rundy={set(r for _,r in s_k)}")

# ─── graph layout visualizations ───────────────────────────────────────────────

print("\n--- Generowanie wizualizacji struktury grafów ---")

def draw_graph_structure(g, n, layout_pos, title, filename, highlight_nodes=None):
    G = nx.Graph()
    G.add_nodes_from(range(n))
    for u in range(n):
        for v in range(u+1, n):
            if g.has_edge(u, v):
                G.add_edge(u, v)

    fig, ax = plt.subplots(figsize=(5, 4))
    node_colors = ['#e74c3c' if (highlight_nodes and i in highlight_nodes) else '#3498db' for i in range(n)]
    nx.draw_networkx(G, layout_pos, ax=ax,
                     node_color=node_colors, node_size=700,
                     font_color='white', font_weight='bold', font_size=11,
                     edge_color='#555', width=2)
    degrees = [g.degree(i) for i in range(n)]
    for i, (x, y) in layout_pos.items():
        ax.annotate(f"deg={degrees[i]}", (x, y), textcoords='offset points', xytext=(0, -22),
                    ha='center', fontsize=8, color='#333')
    ax.set_title(title, fontsize=12, fontweight='bold')
    ax.axis('off')
    plt.tight_layout()
    plt.savefig(filename, dpi=150, bbox_inches='tight')
    plt.close()
    print(f"  Zapisano: {filename}")

# W_5
g_w5_draw = make_wheel(5)
pos_w5 = {4: (0, 0), 0: (0, 2), 1: (2, 0), 2: (0, -2), 3: (-2, 0)}
draw_graph_structure(g_w5_draw, 5, pos_w5, "Graf kołowy $W_5$", "fig_graph_wheel5.png", highlight_nodes={4})

# K_5 minus edge
g_k5e_draw = make_kn_minus_edge(5, 3, 4)
pos_k5e = nx.circular_layout(nx.complete_graph(5))
pos_k5e = {i: pos for i, pos in enumerate(pos_k5e.values())}
draw_graph_structure(g_k5e_draw, 5, pos_k5e, "$K_5 \\setminus \\{3,4\\}$", "fig_graph_k5e.png", highlight_nodes={3,4})

# Cocktail party CP_3 (n=6)
g_cp3_draw = make_cocktail_party(3)
pos_cp3 = nx.circular_layout(nx.complete_graph(6))
pos_cp3 = {i: p for i, p in enumerate(pos_cp3.values())}
draw_graph_structure(g_cp3_draw, 6, pos_cp3, "Graf koktajlowy $CP_3$ ($n=6$)", "fig_graph_cp3.png")

# Friendship F_2 (n=5)
g_f2_draw = make_friendship(2)
pos_f2 = {4: (0,0), 0: (-1.5, 1), 1: (-1.5, -1), 2: (1.5, 1), 3: (1.5, -1)}
draw_graph_structure(g_f2_draw, 5, pos_f2, "Graf przyjaźni $F_2$ ($n=5$)", "fig_graph_f2.png", highlight_nodes={4})

print("\nWszystkie pliki wygenerowane pomyślnie.")
