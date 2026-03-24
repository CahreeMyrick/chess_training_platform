#include "search/search_bb.hpp"
#include "evaluation/eval.hpp"

#include <limits>
#include <algorithm>

namespace chess {

static int alphabeta_bb(BitboardPosition& pos, int depth, int alpha, int beta) {
    if (depth == 0) {
        return Eval::evaluate(pos);
    }

    MoveList moves;
    pos.generate_legal(moves);

    if (moves.size == 0) {
        if (pos.in_check(pos.side_to_move()))
            return (pos.side_to_move() == Color::White) ? -100000 : +100000;
        return 0; // stalemate
    }

    if (pos.side_to_move() == Color::White) {
        int best = std::numeric_limits<int>::min();
        for (int i = 0; i < moves.size; ++i) {
            pos.do_move(moves.moves[i]);
            int score = alphabeta_bb(pos, depth - 1, alpha, beta);
            pos.undo_move();
            best  = std::max(best, score);
            alpha = std::max(alpha, score);
            if (beta <= alpha) break;
        }
        return best;
    } else {
        int best = std::numeric_limits<int>::max();
        for (int i = 0; i < moves.size; ++i) {
            pos.do_move(moves.moves[i]);
            int score = alphabeta_bb(pos, depth - 1, alpha, beta);
            pos.undo_move();
            best = std::min(best, score);
            beta = std::min(beta, score);
            if (beta <= alpha) break;
        }
        return best;
    }
}

SearchResult SearchBB::minimax(BitboardPosition& pos, int depth) {
    MoveList moves;
    pos.generate_legal(moves);

    SearchResult res;
    res.score = (pos.side_to_move() == Color::White)
        ? std::numeric_limits<int>::min()
        : std::numeric_limits<int>::max();

    for (int i = 0; i < moves.size; ++i) {
        pos.do_move(moves.moves[i]);
        int score = alphabeta_bb(
            pos, depth - 1,
            std::numeric_limits<int>::min(),
            std::numeric_limits<int>::max()
        );
        pos.undo_move();

        if (pos.side_to_move() == Color::White) {
            if (score > res.score) { res.score = score; res.best = moves.moves[i]; }
        } else {
            if (score < res.score) { res.score = score; res.best = moves.moves[i]; }
        }
    }

    return res;
}

} // namespace chess
