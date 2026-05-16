#!/usr/bin/env python3
"""OpenMP speedup benchmark — HATS Solver.

Uruchomienie:
  python3 benchmark.py

Mierzy czas Solver.run() dla K_n (n=10..20) przy różnych OMP_NUM_THREADS.
Każda konfiguracja jest uruchamiana w osobnym subprocesie z ustawionym
OMP_NUM_THREADS, żeby OpenMP poprawnie zainicjalizował liczbę wątków.

Wynik: tabela w terminalu + benchmark_speedup.png
"""
import json
import multiprocessing
import os
import subprocess
import sys
import time
from pathlib import Path

try:
    import matplotlib.pyplot as plt
    import numpy as np
    HAS_PLOT = True
except ImportError:
    HAS_PLOT = False
    print("Uwaga: matplotlib niedostępny — wykres nie zostanie zapisany.")

# ── parametry ────────────────────────────────────────────────────────────
N_VALUES      = [10, 12, 14, 16, 18, 20]
THREAD_COUNTS = [1, 2, 4, 8]
REPS          = 7          # powtórzeń per punkt — bierzemy medianę
BUILD_DIR     = str(Path(__file__).parent / "build")


def alternating_world(n: int) -> int:
    """Bity parzyste = 1, nieparzyste = 0 — gwarantuje nietrywialną symulację."""
    return sum(1 << i for i in range(0, n, 2))


# Skrypt Python uruchamiany w subprocesie z danym OMP_NUM_THREADS
_MEASURE_SCRIPT = """\
import sys, time, json
sys.path.insert(0, {build!r})
import hats_py

g = hats_py.Graph.make_complete({n})
world = {world}
times = []
for _ in range({reps}):
    t0 = time.perf_counter()
    hats_py.Solver(g, world).run()
    times.append(time.perf_counter() - t0)
times.sort()
median = times[len(times) // 2]
minimum = times[0]
print(json.dumps({{"median": median, "min": minimum, "reps": {reps}}}))
"""


def measure(n: int, threads: int) -> dict:
    world = alternating_world(n)
    script = _MEASURE_SCRIPT.format(build=BUILD_DIR, n=n, world=world, reps=REPS)
    env = os.environ.copy()
    env["OMP_NUM_THREADS"] = str(threads)
    proc = subprocess.run(
        [sys.executable, "-c", script],
        env=env, capture_output=True, text=True,
    )
    if proc.returncode != 0:
        raise RuntimeError(proc.stderr[:300])
    return json.loads(proc.stdout)


def print_table(ns, threads, results):
    col_w = 10

    # ── czas [ms] ──────────────────────────────────────────────────────
    header = f"{'n':>4}  " + "  ".join(f"T={t:>{col_w-2}}" for t in threads)
    print("\nCzas mediany [ms]:")
    print(header)
    print("─" * len(header))
    for n in ns:
        row = f"{n:>4}  "
        for t in threads:
            val = results.get((n, t))
            row += f"  {val*1000:>{col_w-2}.2f}ms" if val is not None else f"  {'—':>{col_w}}"
        print(row)

    # ── speedup ────────────────────────────────────────────────────────
    multi = [t for t in threads if t > 1]
    if not multi:
        return
    header2 = f"{'n':>4}  " + "  ".join(f"T={t:>{col_w-2}}" for t in multi)
    print("\nSpeedup vs. 1 wątek:")
    print(header2)
    print("─" * len(header2))
    for n in ns:
        base = results.get((n, 1))
        if base is None:
            continue
        row = f"{n:>4}  "
        for t in multi:
            val = results.get((n, t))
            sp = base / val if val else None
            row += f"  {sp:>{col_w-2}.2f}×" if sp else f"  {'—':>{col_w}}"
        print(row)


def plot(ns, threads, results, save_only=False):
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(13, 5))
    fig.suptitle(
        "HATS Solver — OpenMP Speedup Benchmark\n"
        f"(K_n complete, świat=alternating, mediana {REPS} powtórzeń)",
        fontsize=11,
    )
    colors = plt.cm.tab10(np.linspace(0, 0.7, len(threads)))

    # lewy: czas vs n, po jednej linii na liczbę wątków
    for color, t in zip(colors, threads):
        xs = [n for n in ns if (n, t) in results]
        ys = [results[(n, t)] * 1000 for n in xs]
        ax1.plot(xs, ys, "o-", color=color, label=f"{t} wąt.")
    ax1.set_xlabel("n (liczba agentów)")
    ax1.set_ylabel("Czas [ms] — skala log")
    ax1.set_title("Czas wykonania")
    ax1.set_yscale("log")
    ax1.legend()
    ax1.grid(True, alpha=0.3)

    # prawy: speedup vs n
    multi = [t for t in threads if t > 1]
    colors2 = plt.cm.tab10(np.linspace(0.1, 0.8, len(multi)))
    for color, t in zip(colors2, multi):
        xs = [n for n in ns if (n, 1) in results and (n, t) in results]
        ys = [results[(n, 1)] / results[(n, t)] for n in xs]
        ax2.plot(xs, ys, "s-", color=color, label=f"{t} wątki")
    ax2.axhline(1.0, color="gray", linestyle="--", linewidth=0.8, label="baseline")
    ax2.set_xlabel("n (liczba agentów)")
    ax2.set_ylabel("Speedup (vs. 1 wątek)")
    ax2.set_title("Przyspieszenie OpenMP")
    ax2.legend()
    ax2.grid(True, alpha=0.3)

    plt.tight_layout()
    out = "benchmark_speedup.png"
    fig.savefig(out, dpi=120, bbox_inches="tight")
    print(f"\nWykres: {out}")
    if not save_only:
        plt.show()


def main():
    max_cores = multiprocessing.cpu_count()
    threads = sorted({t for t in THREAD_COUNTS if t <= max_cores} | {max_cores})

    print(f"HATS OpenMP Speedup Benchmark")
    print(f"Rdzenie CPU: {max_cores}  |  Konfiguracje wątków: {threads}")
    print(f"n={N_VALUES}  reps={REPS}  topologia=K_n (complete)")
    print()

    results: dict[tuple[int, int], float] = {}
    total = len(N_VALUES) * len(threads)
    done = 0

    for n in N_VALUES:
        for t in threads:
            done += 1
            print(f"[{done:>3}/{total}] n={n:>2} T={t} ...", end="", flush=True)
            try:
                data = measure(n, t)
                results[(n, t)] = data["median"]
                print(f"  {data['median']*1000:6.2f} ms")
            except Exception as exc:
                print(f"  BŁĄD: {exc}")

    print_table(N_VALUES, threads, results)

    if HAS_PLOT:
        plot(N_VALUES, threads, results, save_only=sys.stdout.isatty() is False)


if __name__ == "__main__":
    main()
