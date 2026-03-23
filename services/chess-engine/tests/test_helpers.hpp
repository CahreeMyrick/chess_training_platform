#pragma once

#include "core/types.hpp"
#include "core/move.hpp"
#include "position/position.hpp"
#include "position/board_factory.hpp"
#include "render/render.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <cstdio>

namespace chess::test {

// Set to true by --visualize flag to print board after each move
extern bool g_visualize;

// Convert "e2" -> square index using make_sq
inline int sq(const char* s) {
    int f = s[0] - 'a';
    int r = s[1] - '1';
    return make_sq(f, r);
}

// Build a Move from algebraic square names
inline Move mv(const char* from, const char* to, uint8_t promo = PROMO_NONE) {
    return Move{(uint8_t)sq(from), (uint8_t)sq(to), promo};
}

// Create starting position backed by ArrayBoard
inline Position startpos() {
    return Position::startpos(make_board(BoardType::Array));
}

// Create an empty position with given side to move
inline Position empty_pos(Color stm = Color::White) {
    auto pos = Position(make_board(BoardType::Array));
    pos.set_side_to_move(stm);
    return pos;
}

// Print the board when visualization is enabled
inline void show(const Position& pos, const std::string& label = "") {
    if (!g_visualize) return;
    if (!label.empty()) {
        std::cout << "\033[1;33m" << label << "\033[0m\n";
    }
    std::cout << Render::board_ascii(pos) << std::flush;
}

// Play a list of moves, printing the board after each if visualization is on.
// annotations[i] is printed before move i (optional).
// Returns false (and prints error) if any move is rejected.
inline bool play_moves(
    Position& pos,
    const std::vector<Move>& moves,
    const std::vector<std::string>& annotations = {}
) {
    for (int i = 0; i < (int)moves.size(); ++i) {
        if (g_visualize) {
            std::string label;
            if (i < (int)annotations.size()) {
                label = annotations[i];
            } else {
                char buf[64];
                std::snprintf(buf, sizeof(buf), "Move %d (%s)",
                    (i / 2) + 1, (i % 2 == 0) ? "White" : "Black");
                label = buf;
            }
            std::cout << "\033[1;36m--- " << label << " ---\033[0m\n";
        }

        std::string err;
        if (!pos.make_move(moves[i], err)) {
            if (g_visualize) {
                std::cerr << "\033[1;31mMove failed: " << err << "\033[0m\n";
            }
            return false;
        }

        show(pos);
    }
    return true;
}

} // namespace chess::test
