#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "position/board_factory.hpp"
#include "position/position.hpp"
#include "position/bb_position.hpp"
#include "search/search.hpp"
#include "search/search_bb.hpp"
#include "core/move.hpp"
#include "core/types.hpp"

namespace chess {

static std::string square_to_string(int sq) {
    char file = static_cast<char>('a' + file_of(sq));
    char rank = static_cast<char>('1' + rank_of(sq));

    std::string out;
    out.push_back(file);
    out.push_back(rank);
    return out;
}

static std::string move_to_string(const Move& m) {
    std::string s = square_to_string(m.from) + " " + square_to_string(m.to);

    if (m.promo != PROMO_NONE) {
        char promo_char = '?';
        switch (m.promo) {
            case PROMO_Q: promo_char = 'q'; break;
            case PROMO_R: promo_char = 'r'; break;
            case PROMO_B: promo_char = 'b'; break;
            case PROMO_N: promo_char = 'n'; break;
            default:      promo_char = '?'; break;
        }
        s += " ";
        s.push_back(promo_char);
    }

    return s;
}

static const char* board_type_name(BoardType type) {
    switch (type) {
        case BoardType::Array:    return "ArrayBoard";
        case BoardType::Pointer:  return "PointerBoard";
        case BoardType::Bitboard: return "Bitboard";
        default:                  return "Unknown";
    }
}

static void run_one(BoardType type, int depth, int repetitions) {
    using clock = std::chrono::steady_clock;

    long long total_ms = 0;
    SearchResult last_result{};

    for (int i = 0; i < repetitions; ++i) {
        auto board = make_board(type);
        if (!board) {
            std::cout << board_type_name(type) << ": not implemented\n";
            return;
        }

        Position pos = Position::startpos(std::move(board));

        auto t0 = clock::now();
        last_result = Search::minimax(pos, depth);
        auto t1 = clock::now();

        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
        total_ms += ms;
    }

    double avg_ms = static_cast<double>(total_ms) / repetitions;

    std::cout << board_type_name(type)
              << " | depth=" << depth
              << " | reps=" << repetitions
              << " | best=" << move_to_string(last_result.best)
              << " | score=" << last_result.score
              << " | avg_ms=" << avg_ms
              << "\n";
}

static void run_bb(int depth, int repetitions) {
    using clock = std::chrono::steady_clock;

    long long total_ms = 0;
    SearchResult last_result{};

    for (int i = 0; i < repetitions; ++i) {
        BitboardPosition pos = BitboardPosition::startpos();

        auto t0 = clock::now();
        last_result = SearchBB::minimax(pos, depth);
        auto t1 = clock::now();

        total_ms += std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    }

    double avg_ms = static_cast<double>(total_ms) / repetitions;

    std::cout << "BitboardPosition | depth=" << depth
              << " | reps=" << repetitions
              << " | best=" << move_to_string(last_result.best)
              << " | score=" << last_result.score
              << " | avg_ms=" << avg_ms
              << "\n";
}

} // namespace chess

int main() {
    using namespace chess;

    int depth = 5;
    int repetitions = 3;

    std::cout << "Backend comparison benchmark (depth=" << depth << ")\n";
    std::cout << "-----------------------------------------------\n";

    run_one(BoardType::Array, depth, repetitions);
    run_one(BoardType::Pointer, depth, repetitions);
    run_one(BoardType::Bitboard, depth, repetitions);
    run_bb(depth, repetitions);

    return 0;
}