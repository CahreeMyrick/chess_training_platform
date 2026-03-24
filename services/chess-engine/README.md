# Chess Engine

A C++17 chess engine with three swappable board backends and a bitboard-native search path.

## Building

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Requires CMake 3.16+ and a C++17 compiler. GoogleTest is fetched automatically at configure time.

## Running

```bash
# Interactive search (defaults to ArrayBoard)
./build/chess_engine

# Choose a backend explicitly
./build/chess_engine array
./build/chess_engine pointer
./build/chess_engine bitboard

# Backend comparison benchmark
./build/chess_benchmark

# Test suite
./build/chess_tests
```

## Architecture

```
src/
├── attacks/          # Precomputed attack tables + sliding ray functions
├── core/             # Types, Move, MoveList
├── evaluation/       # Material evaluation
├── movegen/          # Pseudo-legal and legal move generation (Position-based)
├── position/         # Board backends + position wrappers
├── render/           # ASCII board display
├── rules/            # Move validation and check detection
├── scripts/          # main.cpp, benchmark_main.cpp entry points
└── search/           # Alpha-beta search (two paths — see below)
```

## Board Backends

Three backends implement the same `Board` interface and are selected at runtime via `make_board(BoardType)`.

| Backend | Storage | `piece_at` | Notes |
|---------|---------|-----------|-------|
| `ArrayBoard` | `array<Piece, 64>` | O(1) | Simple baseline |
| `PointerBoard` | `array<unique_ptr<PieceObject>, 64>` | O(1) + vtable | Object-oriented, slowest |
| `Bitboard` | 12 × `uint64_t` + mailbox + occupancy | O(1) | Upgraded in Phase 2 |

The `Bitboard` backend maintains three additional structures alongside its 12 piece bitboards:
- `mailbox_[64]` — direct piece lookup, eliminates the previous O(12) sequential scan
- `occ_[2]` — per-color occupancy bitboards
- `occ_all_` — full-board occupancy

## BitboardPosition

`BitboardPosition` is a self-contained, bitboard-native position type separate from the `Board`/`Position` hierarchy. It is the fast search path.

Key differences from `Position`:

**Undo stack instead of position copies.** `do_move(Move)` pushes a small `State` struct (castling rights, en-passant square, captured piece, EP flag) and updates the board in place. `undo_move()` pops the state and reverses every change in O(1). The existing `Position`-based search copies the entire position at every node.

**Bitboard-native move generation.** Pawns are generated with whole-board shifts on the pawn bitboard — one shift produces all single pushes simultaneously, rather than iterating squares. Knights and kings use precomputed attack tables. Sliders use on-the-fly ray functions with occupancy masking.

**Pure-bitboard attack detection.** `square_attacked(sq, by)` uses bitboard intersection throughout — no square scanning at any point.

## Attacks Module

`src/attacks/attacks.hpp` exposes:

```cpp
attacks::knight(sq)          // precomputed, O(1)
attacks::king(sq)            // precomputed, O(1)
attacks::pawn(color, sq)     // precomputed, O(1)
attacks::bishop_otf(sq, occ) // on-the-fly ray cast
attacks::rook_otf(sq, occ)   // on-the-fly ray cast
```

Tables are populated via a static constructor before `main()` — no explicit `init()` call is needed. `Rules::is_square_attacked` uses the knight and king tables to replace O(64) board scans with O(8) iterations.

The `bishop_otf` / `rook_otf` names signal that these are the stepping-stone implementations before magic bitboards. The interface is already defined for the future upgrade: swap the internals, callers don't change.

## Search

Two search paths exist side by side:

| Class | Position type | Node cost | Use |
|-------|--------------|-----------|-----|
| `Search` | `Position` (copy-per-node) | O(board size) per node | All three `Board` backends |
| `SearchBB` | `BitboardPosition` (undo stack) | O(1) per node | Fast path |

Both use the same alpha-beta algorithm with the same evaluation function, so they produce identical results.

**Benchmark results at depth 5 from startpos:**

| Backend | avg ms | vs ArrayBoard |
|---------|--------|---------------|
| ArrayBoard | ~101 ms | baseline |
| Bitboard (Search path) | ~108 ms | ~7% slower |
| PointerBoard | ~728 ms | 7.2× slower |
| **BitboardPosition (SearchBB)** | **~18 ms** | **5.6× faster** |

The Bitboard backend sitting near ArrayBoard confirms the copy overhead in `Search` was the bottleneck, not the board representation. `SearchBB` eliminates that overhead entirely.

## Test Suite

61 tests across 5 suites covering position state, move generation, rules, evaluation, and full game sequences (Fool's Mate, Scholar's Mate, castling, en passant, promotion, stalemate).

```bash
./build/chess_tests
./build/chess_tests --gtest_filter="SequencesTest.*"
```

## Roadmap

- **Magic bitboards** — replace `bishop_otf` / `rook_otf` with precomputed magic lookup tables. Interface is already in place; no callers change. Worth doing when slider-heavy positions become the bottleneck (measurable at depth 6+).
- **Move ordering** — MVV-LLA capture ordering and killer moves to improve alpha-beta cutoff rate.
- **Piece-square tables** — positional bonuses added to `Eval::evaluate(BitboardPosition)`.
- **Iterative deepening** — time-controlled search built on top of `SearchBB`.
