import sys
sys.path.insert(0, 'build')
import hats_py as hp
import matplotlib.pyplot as plt
import matplotlib
import numpy as np

matplotlib.rcParams.update({'font.size': 11, 'figure.dpi': 150})


def make_k_bipartite(m, n):
    g = hp.Graph(m + n)
    for u in range(m):
        for v in range(m, m + n):
            g.add_edge(u, v)
    return g


def make_petersen():
    g = hp.Graph(10)
    for i in range(5):
        g.add_edge(i, (i + 1) % 5)
    for i in range(5):
        g.add_edge(i + 5, ((i + 2) % 5) + 5)
    for i in range(5):
        g.add_edge(i, i + 5)
    return g


def make_kne(n, u, v):
    g = hp.Graph(n)
    for a in range(n):
        for b in range(a + 1, n):
            if (a == u and b == v) or (a == v and b == u):
                continue
            g.add_edge(a, b)
    return g


def make_wheel(n):
    g = hp.Graph(n)
    for i in range(n - 1):
        g.add_edge(i, (i + 1) % (n - 1))
        g.add_edge(i, n - 1)
    return g


def run_exhaustive(g):
    n = g.size()
    total = (1 << n) - 1
    successes, deadlocks = [], []
    for w in range(1, total + 1):
        r = hp.Solver(g, w).run()
        if r.success:
            successes.append(w)
        else:
            deadlocks.append(w)
    return successes, deadlocks


def plot_entropy(g, world, title, filename, max_rounds=None):
    n = g.size()
    records = hp.Solver(g, world).trace()
    if not records:
        print(f"No records for {filename}")
        return
    if max_rounds is not None:
        records = records[:max_rounds]
    rounds = [r.round for r in records]
    entropies = [[np.log2(max(c, 1)) for c in r.agent_class_sizes] for r in records]
    fig, ax = plt.subplots(figsize=(7, 3.5))
    for i in range(n):
        values = [e[i] if i < len(e) else 0 for e in entropies]
        ax.plot(rounds, values, marker='o', label=f'Agent {i}', linewidth=1.5)
    ax.set_xlabel('Runda')
    ax.set_ylabel(r'$H_i(t) = \log_2|[i,v] \cap W_\mathrm{valid}(t)|$')
    ax.set_title(title)
    ax.legend(fontsize=7, ncol=max(1, n // 4), loc='upper right')
    ax.grid(True, alpha=0.3)
    plt.tight_layout()
    plt.savefig(filename, dpi=150, bbox_inches='tight')
    plt.close()
    print(f"  Saved {filename}")


# ── K_{m,n} bipartite ────────────────────────────────────────────────────────
print("=== K_{m,n} bipartite ===")
results_bipartite = {}
for (m, n) in [(2, 2), (2, 3), (3, 3), (2, 4), (3, 4)]:
    g = make_k_bipartite(m, n)
    succ, dead = run_exhaustive(g)
    total = (1 << (m + n)) - 1
    results_bipartite[(m, n)] = (len(succ), total)
    print(f"  K_{{{m},{n}}}: {len(succ)}/{total} successes")

# Entropy deadlock plot for K_{3,3}
g33 = make_k_bipartite(3, 3)
print("  Plotting K_{3,3} entropy...")
plot_entropy(g33, 1, r'$K_{3,3}$ — zakleszczenie ($w = 000001_2$)', 'fig_entropy_k33_dead.png', max_rounds=3)

# ── Petersen ─────────────────────────────────────────────────────────────────
print("=== Petersen graph ===")
gp = make_petersen()
succ_p, dead_p = [], []
# Exhaustive (2^10 - 1 = 1023 worlds, n=10 is small)
print("  Running exhaustive Petersen (1023 worlds)...")
for w in range(1, 1024):
    r = hp.Solver(gp, w).run()
    if r.success:
        succ_p.append(w)
    else:
        dead_p.append(w)
print(f"  Petersen: {len(succ_p)}/1023 successes")

plot_entropy(gp, 1, r'Graf Petersena — zakleszczenie ($w = 0000000001_2$)', 'fig_entropy_petersen_dead.png', max_rounds=3)

# ── Extended K_n\e ───────────────────────────────────────────────────────────
print("=== Extended K_n\\e ===")
kne_results = {}
for n in [4, 5, 6, 7, 8]:
    u, v = n - 2, n - 1
    g = make_kne(n, u, v)
    succ, dead = run_exhaustive(g)
    total = (1 << n) - 1
    expected = (1 << (n - 2)) - 1
    kne_results[n] = (len(succ), total, expected)
    match = "OK" if len(succ) == expected else "MISMATCH"
    pct = 100 * len(succ) / total
    print(f"  K_{n}\\e: {len(succ)}/{total} ({pct:.1f}%), expected={expected} [{match}]")

# ── Extended W_n ─────────────────────────────────────────────────────────────
print("=== Extended W_n ===")
wheel_results = {}
for wn in [4, 5, 6, 7, 8, 9, 10]:
    g = make_wheel(wn)
    succ, dead = run_exhaustive(g)
    total = (1 << wn) - 1
    center_world = 1 << (wn - 1)
    wheel_results[wn] = (len(succ), total)
    print(f"  W_{wn}: {len(succ)}/{total} successes, center={center_world} in succ={center_world in succ}")

# ── Updated success rate bar chart ───────────────────────────────────────────
print("=== Generating updated bar chart ===")

labels = [
    r'$K_4$',
    r'$K_5$',
    r'$K_5\!\setminus\!e$',
    r'$K_6\!\setminus\!e$',
    r'$K_{3,3}$',
    'Petersen',
    r'$W_4{=}K_4$',
    r'$W_5$',
    r'$W_6$',
    r'$S_4$',
    r'$S_5$',
    r'$C_4^+$',
    r'$C_5$',
    r'$Q_3$',
]

succ_counts = [
    15, 31,
    kne_results[5][0], kne_results[6][0],
    results_bipartite[(3, 3)][0],
    len(succ_p),
    15, 1, 1,
    1, 1,
    3,
    0, 0,
]
total_counts = [
    15, 31,
    kne_results[5][1], kne_results[6][1],
    results_bipartite[(3, 3)][1],
    1023,
    15, 31, 63,
    15, 31,
    15,
    31, 255,
]
rates = [100 * s / t for s, t in zip(succ_counts, total_counts)]

colors = []
for r in rates:
    if r >= 99:
        colors.append('#2ecc71')
    elif r >= 20:
        colors.append('#f39c12')
    elif r > 0:
        colors.append('#e67e22')
    else:
        colors.append('#e74c3c')

fig, ax = plt.subplots(figsize=(12, 5))
bars = ax.bar(labels, rates, color=colors, edgecolor='white', linewidth=0.8)
for bar, rate in zip(bars, rates):
    ypos = bar.get_height() + 1.5
    ax.text(bar.get_x() + bar.get_width() / 2, ypos, f'{rate:.0f}%',
            ha='center', va='bottom', fontsize=9)
ax.set_ylabel('Odsetek sukcesów [%]')
ax.set_title('Odsetek światów kończących się sukcesem dla różnych topologii grafowych')
ax.set_ylim(0, 115)
ax.tick_params(axis='x', labelsize=9)
ax.grid(axis='y', alpha=0.3)

from matplotlib.patches import Patch
legend_elements = [
    Patch(facecolor='#2ecc71', label='100% (pełny sukces)'),
    Patch(facecolor='#f39c12', label='20–99% (częściowy sukces)'),
    Patch(facecolor='#e67e22', label='1–19% (rzadki sukces)'),
    Patch(facecolor='#e74c3c', label='0% (zawsze zakleszczenie)'),
]
ax.legend(handles=legend_elements, fontsize=9, loc='upper right')
plt.tight_layout()
plt.savefig('fig_success_rates.png', dpi=150, bbox_inches='tight')
plt.close()
print("  Saved fig_success_rates.png")

# ── Summary table ─────────────────────────────────────────────────────────────
print("\n=== Summary for LaTeX ===")
print("K_{m,n} results:")
for (m, n), (s, t) in results_bipartite.items():
    print(f"  K_{{{m},{n}}}: {s}/{t} ({100*s/t:.1f}%)")

print("\nK_n\\e extended:")
for n, (s, t, e) in kne_results.items():
    print(f"  n={n}: {s}/{t} ({100*s/t:.1f}%), 2^(n-2)-1={e}")

print("\nW_n extended:")
for wn, (s, t) in wheel_results.items():
    print(f"  W_{wn}: {s}/{t} ({100*s/t:.2f}%)")

print("\nPetersen: 0/1023 (0%)")
