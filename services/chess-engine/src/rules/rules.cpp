#include "rules/rules.hpp"
#include "attacks/attacks.hpp"
#include <cstdlib>

namespace chess {

static int dr(int from, int to) { return rank_of(to) - rank_of(from); }
static int df(int from, int to) { return file_of(to) - file_of(from); }

static bool has_right(uint8_t rights, uint8_t flag) {
    return (rights & flag) != 0;
}

bool Rules::is_friend(Piece moving, Piece target) {
    if (is_empty(target)) return false;
    return (is_white(moving) && is_white(target)) ||
           (is_black(moving) && is_black(target));
}

bool Rules::is_opponent(Piece moving, Piece target) {
    if (is_empty(target)) return false;
    return (is_white(moving) && is_black(target)) ||
           (is_black(moving) && is_white(target));
}

bool Rules::path_clear(const Position& pos, int from, int to) {
    int r0 = rank_of(from), f0 = file_of(from);
    int r1 = rank_of(to),   f1 = file_of(to);

    int step_r = (r1 > r0) ? 1 : (r1 < r0 ? -1 : 0);
    int step_f = (f1 > f0) ? 1 : (f1 < f0 ? -1 : 0);

    int r = r0 + step_r;
    int f = f0 + step_f;

    while (r != r1 || f != f1) {
        int sq = make_sq(f, r);
        if (!is_empty(pos.at(sq))) return false;
        r += step_r;
        f += step_f;
    }

    return true;
}

bool Rules::pseudo_knight(const Position& pos, Piece p, int from, int to, std::string& err) {
    int r = std::abs(dr(from, to));
    int f = std::abs(df(from, to));

    if (!((r == 2 && f == 1) || (r == 1 && f == 2))) {
        err = "Knight moves in an L.";
        return false;
    }

    Piece target = pos.at(to);
    if (is_friend(p, target)) {
        err = "Cannot capture your own piece.";
        return false;
    }

    return true;
}

bool Rules::pseudo_bishop(const Position& pos, Piece p, int from, int to, std::string& err) {
    int r = std::abs(dr(from, to));
    int f = std::abs(df(from, to));

    if (r == 0 || f == 0 || r != f) {
        err = "Bishop moves diagonally.";
        return false;
    }

    if (!path_clear(pos, from, to)) {
        err = "Bishop path is blocked.";
        return false;
    }

    Piece target = pos.at(to);
    if (is_friend(p, target)) {
        err = "Cannot capture your own piece.";
        return false;
    }

    return true;
}

bool Rules::pseudo_rook(const Position& pos, Piece p, int from, int to, std::string& err) {
    int r = dr(from, to);
    int f = df(from, to);

    if (!((r == 0 && f != 0) || (f == 0 && r != 0))) {
        err = "Rook moves in straight lines.";
        return false;
    }

    if (!path_clear(pos, from, to)) {
        err = "Rook path is blocked.";
        return false;
    }

    Piece target = pos.at(to);
    if (is_friend(p, target)) {
        err = "Cannot capture your own piece.";
        return false;
    }

    return true;
}

bool Rules::pseudo_queen(const Position& pos, Piece p, int from, int to, std::string& err) {
    int r = std::abs(dr(from, to));
    int f = std::abs(df(from, to));

    bool diag = (r != 0 && f != 0 && r == f);
    bool straight = (r == 0 && f != 0) || (f == 0 && r != 0);

    if (!(diag || straight)) {
        err = "Queen moves like rook or bishop.";
        return false;
    }

    if (!path_clear(pos, from, to)) {
        err = "Queen path is blocked.";
        return false;
    }

    Piece target = pos.at(to);
    if (is_friend(p, target)) {
        err = "Cannot capture your own piece.";
        return false;
    }

    return true;
}

static bool is_adjacent_king_move(int from, int to) {
    int rd = std::abs(rank_of(to) - rank_of(from));
    int fd = std::abs(file_of(to) - file_of(from));
    return (rd <= 1 && fd <= 1) && !(rd == 0 && fd == 0);
}

bool Rules::pseudo_king(const Position& pos, Piece p, int from, int to, std::string& err) {
    if (rank_of(from) == rank_of(to) && std::abs(file_of(to) - file_of(from)) == 2) {
        Move m;
        m.from = static_cast<uint8_t>(from);
        m.to   = static_cast<uint8_t>(to);
        m.promo = PROMO_NONE;

        std::string e2;
        if (!castle_path_safe(pos, m, e2)) {
            err = e2;
            return false;
        }
        return true;
    }

    if (!is_adjacent_king_move(from, to)) {
        err = "King moves one square in any direction.";
        return false;
    }

    Piece target = pos.at(to);
    if (is_friend(p, target)) {
        err = "Cannot capture your own piece.";
        return false;
    }

    return true;
}

bool Rules::pseudo_pawn(const Position& pos, Piece p, const Move& m, std::string& err) {
    int from = m.from;
    int to   = m.to;

    bool white = is_white(p);
    int dir = white ? +1 : -1;
    int start_rank = white ? 1 : 6;
    int last_rank  = white ? 7 : 0;

    bool promotes = (rank_of(to) == last_rank);

    if (!promotes && m.promo != PROMO_NONE) {
        err = "Promotion choice only allowed when reaching last rank.";
        return false;
    }

    if (promotes) {
        if (!(m.promo == PROMO_NONE ||
              m.promo == PROMO_Q ||
              m.promo == PROMO_R ||
              m.promo == PROMO_B ||
              m.promo == PROMO_N)) {
            err = "Invalid promotion piece.";
            return false;
        }
    }

    int r = dr(from, to);
    int f = df(from, to);
    Piece target = pos.at(to);

    if (f == 0) {
        if (r == dir) {
            if (!is_empty(target)) {
                err = "Pawn forward move must land on empty.";
                return false;
            }
            return true;
        }

        if (r == 2 * dir) {
            if (rank_of(from) != start_rank) {
                err = "Pawn double-step only from start rank.";
                return false;
            }
            if (!is_empty(target)) {
                err = "Pawn double-step must land on empty.";
                return false;
            }

            int mid = make_sq(file_of(from), rank_of(from) + dir);
            if (!is_empty(pos.at(mid))) {
                err = "Pawn double-step is blocked.";
                return false;
            }

            return true;
        }

        err = "Invalid pawn forward move distance.";
        return false;
    }

    if (std::abs(f) == 1 && r == dir) {
        if (is_opponent(p, target)) return true;

        if (is_empty(target) && pos.ep_square() == to) {
            int cap_r = rank_of(to) - dir;
            int cap_f = file_of(to);
            int cap_sq = make_sq(cap_f, cap_r);
            Piece cap = pos.at(cap_sq);

            if (white && cap == Piece::BP) return true;
            if (!white && cap == Piece::WP) return true;

            err = "En passant square set but no capturable pawn found.";
            return false;
        }

        err = "Pawn diagonal must capture (or be en passant).";
        return false;
    }

    err = "Invalid pawn move.";
    return false;
}

bool Rules::is_pseudo_legal(const Position& pos, const Move& m, std::string& err) {
    err.clear();

    if (m.from > 63 || m.to > 63) {
        err = "Move squares out of range.";
        return false;
    }

    if (m.from == m.to) {
        err = "From and to squares are the same.";
        return false;
    }

    Piece moving = pos.at(m.from);
    if (is_empty(moving)) {
        err = "No piece on the from-square.";
        return false;
    }

    if (pos.side_to_move() == Color::White && !is_white(moving)) {
        err = "It's White to move.";
        return false;
    }

    if (pos.side_to_move() == Color::Black && !is_black(moving)) {
        err = "It's Black to move.";
        return false;
    }

    Piece target = pos.at(m.to);
    if (is_friend(moving, target)) {
        err = "Cannot capture your own piece.";
        return false;
    }

    switch (moving) {
        case Piece::WP:
        case Piece::BP:
            return pseudo_pawn(pos, moving, m, err);

        case Piece::WN:
        case Piece::BN:
            return pseudo_knight(pos, moving, m.from, m.to, err);

        case Piece::WB:
        case Piece::BB:
            return pseudo_bishop(pos, moving, m.from, m.to, err);

        case Piece::WR:
        case Piece::BR:
            return pseudo_rook(pos, moving, m.from, m.to, err);

        case Piece::WQ:
        case Piece::BQ:
            return pseudo_queen(pos, moving, m.from, m.to, err);

        case Piece::WK:
        case Piece::BK:
            return pseudo_king(pos, moving, m.from, m.to, err);

        default:
            err = "Unknown piece or not implemented.";
            return false;
    }
}

int Rules::find_king(const Position& pos, Color who) {
    Piece king = (who == Color::White) ? Piece::WK : Piece::BK;

    for (int sq = 0; sq < 64; ++sq) {
        if (pos.at(sq) == king) return sq;
    }

    return -1;
}

static bool adjacent(int a, int b) {
    int rd = std::abs(rank_of(b) - rank_of(a));
    int fd = std::abs(file_of(b) - file_of(a));
    return (rd <= 1 && fd <= 1) && !(rd == 0 && fd == 0);
}

static bool knight_attack(int from, int to) {
    int r = std::abs(rank_of(to) - rank_of(from));
    int f = std::abs(file_of(to) - file_of(from));
    return (r == 2 && f == 1) || (r == 1 && f == 2);
}

bool Rules::is_square_attacked(const Position& pos, int sq, Color by) {
    // Pawns: squares from which a pawn of color 'by' attacks sq are given by
    // pawn(other(by), sq) — the reverse-lookup trick.
    {
        Piece pawn_pc = (by == Color::White) ? Piece::WP : Piece::BP;
        uint64_t from_squares = attacks::pawn(other(by), sq);
        while (from_squares) {
            int from = __builtin_ctzll(from_squares);
            from_squares &= from_squares - 1;
            if (pos.at(from) == pawn_pc) return true;
        }
    }

    // Knights: table lookup limits candidates to at most 8 squares.
    {
        Piece knight = (by == Color::White) ? Piece::WN : Piece::BN;
        uint64_t candidates = attacks::knight(sq);
        while (candidates) {
            int from = __builtin_ctzll(candidates);
            candidates &= candidates - 1;
            if (pos.at(from) == knight) return true;
        }
    }

    // King: table lookup limits candidates to at most 8 squares.
    {
        Piece king = (by == Color::White) ? Piece::WK : Piece::BK;
        uint64_t candidates = attacks::king(sq);
        while (candidates) {
            int from = __builtin_ctzll(candidates);
            candidates &= candidates - 1;
            if (pos.at(from) == king) return true;
        }
    }

    auto ray_hits = [&](int df_step, int dr_step, Piece a, Piece b) -> bool {
        int f = file_of(sq) + df_step;
        int r = rank_of(sq) + dr_step;

        while (f >= 0 && f < 8 && r >= 0 && r < 8) {
            Piece x = pos.at(make_sq(f, r));
            if (!is_empty(x)) return (x == a || x == b);
            f += df_step;
            r += dr_step;
        }
        return false;
    };

    if (by == Color::White) {
        if (ray_hits(+1,+1, Piece::WB, Piece::WQ)) return true;
        if (ray_hits(-1,+1, Piece::WB, Piece::WQ)) return true;
        if (ray_hits(+1,-1, Piece::WB, Piece::WQ)) return true;
        if (ray_hits(-1,-1, Piece::WB, Piece::WQ)) return true;

        if (ray_hits(+1, 0, Piece::WR, Piece::WQ)) return true;
        if (ray_hits(-1, 0, Piece::WR, Piece::WQ)) return true;
        if (ray_hits( 0,+1, Piece::WR, Piece::WQ)) return true;
        if (ray_hits( 0,-1, Piece::WR, Piece::WQ)) return true;
    } else {
        if (ray_hits(+1,+1, Piece::BB, Piece::BQ)) return true;
        if (ray_hits(-1,+1, Piece::BB, Piece::BQ)) return true;
        if (ray_hits(+1,-1, Piece::BB, Piece::BQ)) return true;
        if (ray_hits(-1,-1, Piece::BB, Piece::BQ)) return true;

        if (ray_hits(+1, 0, Piece::BR, Piece::BQ)) return true;
        if (ray_hits(-1, 0, Piece::BR, Piece::BQ)) return true;
        if (ray_hits( 0,+1, Piece::BR, Piece::BQ)) return true;
        if (ray_hits( 0,-1, Piece::BR, Piece::BQ)) return true;
    }

    return false;
}

bool Rules::in_check(const Position& pos, Color who) {
    int ksq = find_king(pos, who);
    if (ksq < 0) return false;
    return is_square_attacked(pos, ksq, other(who));
}

bool Rules::is_castle_move(const Position& pos, const Move& m) {
    Piece moving = pos.at(m.from);
    if (!(moving == Piece::WK || moving == Piece::BK)) return false;

    int r0 = rank_of(m.from), f0 = file_of(m.from);
    int r1 = rank_of(m.to),   f1 = file_of(m.to);

    if (r0 != r1) return false;
    return std::abs(f1 - f0) == 2;
}

bool Rules::castle_path_safe(const Position& pos, const Move& m, std::string& err) {
    err.clear();

    Piece k = pos.at(m.from);
    if (!(k == Piece::WK || k == Piece::BK)) {
        err = "Not a king castling move.";
        return false;
    }

    Color side = (k == Piece::WK) ? Color::White : Color::Black;

    if (Rules::in_check(pos, side)) {
        err = "Cannot castle while in check.";
        return false;
    }

    int from = m.from;
    int to   = m.to;

    int r = rank_of(from);
    int f_from = file_of(from);
    int f_to   = file_of(to);

    bool king_side = (f_to > f_from);
    int through = make_sq(f_from + (king_side ? +1 : -1), r);

    if (Rules::is_square_attacked(pos, through, other(side))) {
        err = "Cannot castle through check.";
        return false;
    }

    if (Rules::is_square_attacked(pos, to, other(side))) {
        err = "Cannot castle into check.";
        return false;
    }

    if (side == Color::White) {
        if (king_side) {
            if (!has_right(pos.castling_rights(), CR_WK)) {
                err = "No white king-side rights.";
                return false;
            }
            if (pos.at(make_sq(5,0)) != Piece::Empty || pos.at(make_sq(6,0)) != Piece::Empty) {
                err = "Squares not empty.";
                return false;
            }
            if (pos.at(make_sq(7,0)) != Piece::WR) {
                err = "Rook missing on h1.";
                return false;
            }
        } else {
            if (!has_right(pos.castling_rights(), CR_WQ)) {
                err = "No white queen-side rights.";
                return false;
            }
            if (pos.at(make_sq(3,0)) != Piece::Empty ||
                pos.at(make_sq(2,0)) != Piece::Empty ||
                pos.at(make_sq(1,0)) != Piece::Empty) {
                err = "Squares not empty.";
                return false;
            }
            if (pos.at(make_sq(0,0)) != Piece::WR) {
                err = "Rook missing on a1.";
                return false;
            }
        }
    } else {
        if (king_side) {
            if (!has_right(pos.castling_rights(), CR_BK)) {
                err = "No black king-side rights.";
                return false;
            }
            if (pos.at(make_sq(5,7)) != Piece::Empty || pos.at(make_sq(6,7)) != Piece::Empty) {
                err = "Squares not empty.";
                return false;
            }
            if (pos.at(make_sq(7,7)) != Piece::BR) {
                err = "Rook missing on h8.";
                return false;
            }
        } else {
            if (!has_right(pos.castling_rights(), CR_BQ)) {
                err = "No black queen-side rights.";
                return false;
            }
            if (pos.at(make_sq(3,7)) != Piece::Empty ||
                pos.at(make_sq(2,7)) != Piece::Empty ||
                pos.at(make_sq(1,7)) != Piece::Empty) {
                err = "Squares not empty.";
                return false;
            }
            if (pos.at(make_sq(0,7)) != Piece::BR) {
                err = "Rook missing on a8.";
                return false;
            }
        }
    }

    return true;
}

} // namespace chess