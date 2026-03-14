# Architecture: Hats

## Core Components

### 1. Types (`types.hpp`)
- `WorldMask`, `VertexMask`, `PlayerId`, `Tick`
- Mask utilities for bit manipulation.

### 2. Graph (`graph.hpp`, `graph.cpp`)
- Optimized for $N \le 64$ using `uint64_t` bitmasks.
- Adjacency list stored as an array of bitmasks.
- No dynamic memory allocation during graph traversal/querying.

## Design Goals
- High performance for state-space exploration.
- Compact memory footprint.
- Safety through basic bounds checking.
