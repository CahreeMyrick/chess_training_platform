#include "search/search.hpp"

#include "movegen/movegen.hpp"
#include "evaluation/eval.hpp"
#include "rules/rules.hpp"

#include <limits>
#include <string>
#include <algorithm>

namespace chess {

static int alphabeta(Position& pos, int depth, int alpha, int beta) {
    if (depth == 0) {
        return Eval::evaluate(pos);
    }

    MoveList moves;
    MoveGen::generate_legal(pos, moves);

    if (moves.size == 0) {
        if (Rules::in_check(pos, pos.side_to_move())) {
            return (pos.side_to_move() == Color::White) ? -100000 : +100000;
        }
        return 0;
    }

    bool maximizing = (pos.side_to_move() == Color::White);

    if (maximizing) {
        int best = std::numeric_limits<int>::min();

        for (int i = 0; i < moves.size; ++i) {
            Position child = pos;
            std::string err;

            if (!child.make_move(moves.moves[i], err)) {
                continue;
            }

            int score = alphabeta(child, depth - 1, alpha, beta);
            best = std::max(best, score);
            alpha = std::max(alpha, score);

            if (beta <= alpha) {
                break;
            }
        }

        return best;
    } else {
        int best = std::numeric_limits<int>::max();

        for (int i = 0; i < moves.size; ++i) {
            Position child = pos;
            std::string err;

            if (!child.make_move(moves.moves[i], err)) {
                continue;
            }

            int score = alphabeta(child, depth - 1, alpha, beta);
            best = std::min(best, score);
            beta = std::min(beta, score);

            if (beta <= alpha) {
                break;
            }
        }

        return best;
    }
}

SearchResult Search::minimax(Position& pos, int depth) {
    MoveList moves;
    MoveGen::generate_legal(pos, moves);

    SearchResult res;
    res.score = (pos.side_to_move() == Color::White)
        ? std::numeric_limits<int>::min()
        : std::numeric_limits<int>::max();

    for (int i = 0; i < moves.size; ++i) {
        Position child = pos;
        std::string err;

        if (!child.make_move(moves.moves[i], err)) {
            continue;
        }

        int score = alphabeta(
            child,
            depth - 1,
            std::numeric_limits<int>::min(),
            std::numeric_limits<int>::max()
        );

        if (pos.side_to_move() == Color::White) {
            if (score > res.score) {
                res.score = score;
                res.best = moves.moves[i];
            }
        } else {
            if (score < res.score) {
                res.score = score;
                res.best = moves.moves[i];
            }
        }
    }

    return res;
}

} // namespace chess
