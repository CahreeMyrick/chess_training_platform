#include "evaluation/eval.hpp"
#include <cstdint>

namespace chess {

static int value(Piece p) {
    switch (p) {
        case Piece::WP: return 100;
        case Piece::WN: return 320;
        case Piece::WB: return 330;
        case Piece::WR: return 500;
        case Piece::WQ: return 900;
        case Piece::WK: return 0;

        case Piece::BP: return -100;
        case Piece::BN: return -320;
        case Piece::BB: return -330;
        case Piece::BR: return -500;
        case Piece::BQ: return -900;
        case Piece::BK: return 0;

        default: return 0;
    }
}

int Eval::evaluate(const Position& pos) {
    int score = 0;

    for (int sq = 0; sq < 64; ++sq) {
        score += value(pos.at(sq));
    }

    return score;
}

// Piece-type indices in BitboardPosition: 0=K 1=Q 2=N 3=B 4=P 5=R
static constexpr int BB_VALUES[6] = {0, 900, 320, 330, 100, 500};

int Eval::evaluate(const BitboardPosition& pos) {
    int score = 0;
    for (int ti = 0; ti < 6; ++ti) {
        score += __builtin_popcountll(pos.pieces(Color::White, ti)) * BB_VALUES[ti];
        score -= __builtin_popcountll(pos.pieces(Color::Black, ti)) * BB_VALUES[ti];
    }
    return score;
}

} // namespace chess