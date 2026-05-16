#!/usr/bin/env python3
"""Sanity-check: K3, actual_world=0b011 (agents 0,1 wear colour 1; agent 2 wears 0)."""
import sys
from pathlib import Path

# The .so is placed in the build directory at the project root.
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "build"))

import hats_py

# --- Graph ---
g = hats_py.Graph.make_complete(3)
print(f"K{g.size()} created")
assert g.size() == 3
assert g.has_edge(0, 1) and g.has_edge(0, 2) and g.has_edge(1, 2)

# --- KnowledgeState (optional internals) ---
ks = hats_py.init_knowledge(g, 0b011)
print(f"world_count={ks.world_count}, actual_views={[hex(v) for v in ks.actual_views]}")

# --- Partitions (optional internals) ---
parts = hats_py.compute_partitions(g, ks.world_count)
print(f"partition classes per agent: {[len(p.classes) for p in parts]}")

# --- Solver ---
solver = hats_py.Solver(g, 0b011)
result = solver.run()
print(result)
assert result.success or result.deadlock, "simulation must terminate"
print("OK")
