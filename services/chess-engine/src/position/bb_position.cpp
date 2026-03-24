#include "position/bb_position.hpp"
#include "attacks/attacks.hpp"
#include <cassert>
#include <cstdlib>   // std::abs

namespace chess {

// Rank / file constants (internal to this file)
static constexpr uint64_t FILE_A = 0x0101010101010101ULL;
static constexpr uint64_t FILE_H = 0x8080808080808080ULL;

static constexpr uint64_t RANK_1 = 0x00000000000000FFULL;
static constexpr uint64_t RANK_2 = 0x000000000000FF00ULL;
static constexpr uint64_t RANK_3 = 0x0000000000FF0000ULL;
static constexpr uint64_t RANK_6 = 0x0000FF0000000000ULL;
static constexpr uint64_t RANK_7 = 0x00FF000000000000ULL;
static constexpr uint64_t RANK_8 = 0xFF00000000000000ULL;

// Pop lowest set bit and return its index
static inline int pop_lsb(uint64_t& b) {
    int sq = __builtin_ctzll(b);
    b &= b - 1;
    return sq;
}

// ============================================================
// Construction
// ============================================================

BitboardPosition::BitboardPosition() {
    for (int i = 0; i < 64; ++i) mailbox_[i] = Piece::Empty;
}

BitboardPosition BitboardPosition::startpos() {
    BitboardPosition pos;
    pos.stm_ = Color::White;
    pos.cr_  = CR_WK | CR_WQ | CR_BK | CR_BQ;
    pos.ep_  = -1;

    for (const auto& pp : STARTPOS_PLACEMENTS) {
        pos.put_piece(pp.pc, pp.sq);
    }

    return pos;
}

// ============================================================
// Internal bitboard primitives
// ============================================================

void BitboardPosition::put_piece(Piece p, int sq) {
    uint64_t b = 1ULL << sq;
    pcs_[piece_ci(p)][piece_ti(p)] |= b;
    occ_[piece_ci(p)] |= b;
    occ_all_ |= b;
    mailbox_[sq] = p;
}

void BitboardPosition::remove_piece(Piece p, int sq) {
    uint64_t mask = ~(1ULL << sq);
    pcs_[piece_ci(p)][piece_ti(p)] &= mask;
    occ_[piece_ci(p)] &= mask;
    occ_all_ &= mask;
    mailbox_[sq] = Piece::Empty;
}

void BitboardPosition::move_piece(Piece p, int from, int to) {
    uint64_t toggle = (1ULL << from) | (1ULL << to);
    pcs_[piece_ci(p)][piece_ti(p)] ^= toggle;
    occ_[piece_ci(p)] ^= toggle;
    occ_all_ ^= toggle;
    mailbox_[from] = Piece::Empty;
    mailbox_[to]   = p;
}

Piece BitboardPosition::promo_piece(Color c, Promotion pr) {
    if (c == Color::White) {
        switch (pr) {
            case PROMO_Q: return Piece::WQ;
            case PROMO_R: return Piece::WR;
            case PROMO_B: return Piece::WB;
            case PROMO_N: return Piece::WN;
            default:      return Piece::WQ;
        }
    } else {
        switch (pr) {
            case PROMO_Q: return Piece::BQ;
            case PROMO_R: return Piece::BR;
            case PROMO_B: return Piece::BB;
            case PROMO_N: return Piece::BN;
            default:      return Piece::BQ;
        }
    }
}

// ============================================================
// Attack detection
// ============================================================

int BitboardPosition::king_square(Color c) const {
    return __builtin_ctzll(pcs_[static_cast<int>(c)][0]);
}

bool BitboardPosition::square_attacked(int sq, Color by) const {
    int ci = static_cast<int>(by);

    // Pawns: use reverse-lookup — pawn(other(by), sq) gives the squares from
    // which a pawn of color 'by' would attack sq.
    if (attacks::pawn(other(by), sq) & pcs_[ci][4]) return true;

    // Knights (table lookup — no board scan)
    if (attacks::knight(sq) & pcs_[ci][2]) return true;

    // King
    if (attacks::king(sq) & pcs_[ci][0]) return true;

    // Bishops + Queens (diagonal rays)
    if (attacks::bishop_otf(sq, occ_all_) & (pcs_[ci][3] | pcs_[ci][1])) return true;

    // Rooks + Queens (straight rays)
    if (attacks::rook_otf(sq, occ_all_) & (pcs_[ci][5] | pcs_[ci][1])) return true;

    return false;
}

bool BitboardPosition::in_check(Color who) const {
    return square_attacked(king_square(who), other(who));
}

// ============================================================
// Move execution (with undo stack)
// ============================================================

void BitboardPosition::do_move(Move m) {
    int from = m.from, to = m.to;
    Piece moving = mailbox_[from];

    State st;
    st.cr       = cr_;
    st.ep       = static_cast<int8_t>(ep_);
    st.captured = Piece::Empty;
    st.moving   = moving;
    st.was_ep   = false;
    st.m        = m;

    // --- En passant capture ---
    if ((moving == Piece::WP || moving == Piece::BP) &&
        ep_ >= 0 && to == ep_ && is_empty(mailbox_[to])) {

        st.was_ep   = true;
        int cap_sq  = (stm_ == Color::White) ? to - 8 : to + 8;
        st.captured = mailbox_[cap_sq];
        remove_piece(st.captured, cap_sq);
        move_piece(moving, from, to);

    } else {
        // --- Normal capture (if any) ---
        if (!is_empty(mailbox_[to])) {
            st.captured = mailbox_[to];
            remove_piece(st.captured, to);
        }

        move_piece(moving, from, to);

        // --- Promotion ---
        if (m.promo != PROMO_NONE) {
            remove_piece(moving, to);
            put_piece(promo_piece(stm_, static_cast<Promotion>(m.promo)), to);
        }

        // --- Castling: move the rook ---
        if ((moving == Piece::WK || moving == Piece::BK) &&
            std::abs(file_of(to) - file_of(from)) == 2) {

            if (stm_ == Color::White) {
                if (to == make_sq(6, 0))
                    move_piece(Piece::WR, make_sq(7, 0), make_sq(5, 0)); // h1 -> f1
                else
                    move_piece(Piece::WR, make_sq(0, 0), make_sq(3, 0)); // a1 -> d1
            } else {
                if (to == make_sq(6, 7))
                    move_piece(Piece::BR, make_sq(7, 7), make_sq(5, 7)); // h8 -> f8
                else
                    move_piece(Piece::BR, make_sq(0, 7), make_sq(3, 7)); // a8 -> d8
            }
        }
    }

    // --- Update castling rights ---
    if (moving == Piece::WK) cr_ &= ~(CR_WK | CR_WQ);
    if (moving == Piece::BK) cr_ &= ~(CR_BK | CR_BQ);
    if (moving == Piece::WR) {
        if (from == make_sq(7, 0)) cr_ &= ~CR_WK;
        if (from == make_sq(0, 0)) cr_ &= ~CR_WQ;
    }
    if (moving == Piece::BR) {
        if (from == make_sq(7, 7)) cr_ &= ~CR_BK;
        if (from == make_sq(0, 7)) cr_ &= ~CR_BQ;
    }
    // Capturing an unmoved rook also forfeits those rights
    if (st.captured == Piece::WR) {
        if (to == make_sq(7, 0)) cr_ &= ~CR_WK;
        if (to == make_sq(0, 0)) cr_ &= ~CR_WQ;
    }
    if (st.captured == Piece::BR) {
        if (to == make_sq(7, 7)) cr_ &= ~CR_BK;
        if (to == make_sq(0, 7)) cr_ &= ~CR_BQ;
    }

    // --- Update en-passant target ---
    ep_ = -1;
    if (moving == Piece::WP && rank_of(to) - rank_of(from) == 2) {
        ep_ = make_sq(file_of(from), rank_of(from) + 1);
    } else if (moving == Piece::BP && rank_of(from) - rank_of(to) == 2) {
        ep_ = make_sq(file_of(from), rank_of(from) - 1);
    }

    stm_ = other(stm_);
    stack_.push_back(st);
}

void BitboardPosition::undo_move() {
    assert(!stack_.empty());
    State st = stack_.back();
    stack_.pop_back();

    stm_ = other(stm_);   // restore side to move
    cr_  = st.cr;
    ep_  = st.ep;

    int from = st.m.from, to = st.m.to;

    // --- Undo castling rook ---
    if ((st.moving == Piece::WK || st.moving == Piece::BK) &&
        std::abs(file_of(to) - file_of(from)) == 2) {

        if (stm_ == Color::White) {
            if (to == make_sq(6, 0))
                move_piece(Piece::WR, make_sq(5, 0), make_sq(7, 0));
            else
                move_piece(Piece::WR, make_sq(3, 0), make_sq(0, 0));
        } else {
            if (to == make_sq(6, 7))
                move_piece(Piece::BR, make_sq(5, 7), make_sq(7, 7));
            else
                move_piece(Piece::BR, make_sq(3, 7), make_sq(0, 7));
        }
    }

    // --- Undo promotion or normal move ---
    if (st.m.promo != PROMO_NONE) {
        // Promoted piece sits at 'to'; replace it with pawn back at 'from'
        remove_piece(mailbox_[to], to);
        put_piece(st.moving, from);
    } else {
        move_piece(st.moving, to, from);
    }

    // --- Restore captured piece ---
    if (st.captured != Piece::Empty) {
        if (st.was_ep) {
            int cap_sq = (stm_ == Color::White) ? to - 8 : to + 8;
            put_piece(st.captured, cap_sq);
        } else {
            put_piece(st.captured, to);
        }
    }
}

// ============================================================
// Move generation — pseudo-legal
// ============================================================

// Helper: push 4 promotion moves for one (from, to) pair
static inline void push_promos(MoveList& out, uint8_t from, uint8_t to) {
    out.push({from, to, PROMO_Q});
    out.push({from, to, PROMO_R});
    out.push({from, to, PROMO_B});
    out.push({from, to, PROMO_N});
}

void BitboardPosition::gen_pawns(MoveList& out) const {
    int ci = static_cast<int>(stm_);
    uint64_t P = pcs_[ci][4];
    uint64_t them_occ = occ_[1 - ci];

    if (stm_ == Color::White) {
        // Single and double pushes
        uint64_t single = (P << 8) & ~occ_all_;
        uint64_t dbl    = ((single & RANK_3) << 8) & ~occ_all_;

        for (uint64_t b = single & ~RANK_8; b;) {
            int to = pop_lsb(b);
            out.push({(uint8_t)(to - 8), (uint8_t)to, PROMO_NONE});
        }
        for (uint64_t b = single & RANK_8; b;) {
            int to = pop_lsb(b);
            push_promos(out, (uint8_t)(to - 8), (uint8_t)to);
        }
        for (uint64_t b = dbl; b;) {
            int to = pop_lsb(b);
            out.push({(uint8_t)(to - 16), (uint8_t)to, PROMO_NONE});
        }

        // Captures NW (<<7) and NE (<<9)
        uint64_t capsNW = ((P & ~FILE_A) << 7) & them_occ;
        uint64_t capsNE = ((P & ~FILE_H) << 9) & them_occ;

        for (uint64_t b = capsNW & ~RANK_8; b;) {
            int to = pop_lsb(b);
            out.push({(uint8_t)(to - 7), (uint8_t)to, PROMO_NONE});
        }
        for (uint64_t b = capsNW & RANK_8; b;) {
            int to = pop_lsb(b);
            push_promos(out, (uint8_t)(to - 7), (uint8_t)to);
        }
        for (uint64_t b = capsNE & ~RANK_8; b;) {
            int to = pop_lsb(b);
            out.push({(uint8_t)(to - 9), (uint8_t)to, PROMO_NONE});
        }
        for (uint64_t b = capsNE & RANK_8; b;) {
            int to = pop_lsb(b);
            push_promos(out, (uint8_t)(to - 9), (uint8_t)to);
        }

        // En passant
        if (ep_ >= 0) {
            uint64_t ep_bb = 1ULL << ep_;
            // Pawn at ep-9 (file-1, rank-1 relative to ep) captures NE
            if (file_of(ep_) > 0 && (P & (ep_bb >> 9)))
                out.push({(uint8_t)(ep_ - 9), (uint8_t)ep_, PROMO_NONE});
            // Pawn at ep-7 (file+1, rank-1 relative to ep) captures NW
            if (file_of(ep_) < 7 && (P & (ep_bb >> 7)))
                out.push({(uint8_t)(ep_ - 7), (uint8_t)ep_, PROMO_NONE});
        }

    } else { // Black
        uint64_t single = (P >> 8) & ~occ_all_;
        uint64_t dbl    = ((single & RANK_6) >> 8) & ~occ_all_;

        for (uint64_t b = single & ~RANK_1; b;) {
            int to = pop_lsb(b);
            out.push({(uint8_t)(to + 8), (uint8_t)to, PROMO_NONE});
        }
        for (uint64_t b = single & RANK_1; b;) {
            int to = pop_lsb(b);
            push_promos(out, (uint8_t)(to + 8), (uint8_t)to);
        }
        for (uint64_t b = dbl; b;) {
            int to = pop_lsb(b);
            out.push({(uint8_t)(to + 16), (uint8_t)to, PROMO_NONE});
        }

        // Captures SE (>>7) and SW (>>9)
        uint64_t capsSE = ((P & ~FILE_H) >> 7) & them_occ;
        uint64_t capsSW = ((P & ~FILE_A) >> 9) & them_occ;

        for (uint64_t b = capsSE & ~RANK_1; b;) {
            int to = pop_lsb(b);
            out.push({(uint8_t)(to + 7), (uint8_t)to, PROMO_NONE});
        }
        for (uint64_t b = capsSE & RANK_1; b;) {
            int to = pop_lsb(b);
            push_promos(out, (uint8_t)(to + 7), (uint8_t)to);
        }
        for (uint64_t b = capsSW & ~RANK_1; b;) {
            int to = pop_lsb(b);
            out.push({(uint8_t)(to + 9), (uint8_t)to, PROMO_NONE});
        }
        for (uint64_t b = capsSW & RANK_1; b;) {
            int to = pop_lsb(b);
            push_promos(out, (uint8_t)(to + 9), (uint8_t)to);
        }

        // En passant
        if (ep_ >= 0) {
            uint64_t ep_bb = 1ULL << ep_;
            // Pawn at ep+9 (file+1, rank+1 relative to ep) captures SW
            if (file_of(ep_) < 7 && (P & (ep_bb << 9)))
                out.push({(uint8_t)(ep_ + 9), (uint8_t)ep_, PROMO_NONE});
            // Pawn at ep+7 (file-1, rank+1 relative to ep) captures SE
            if (file_of(ep_) > 0 && (P & (ep_bb << 7)))
                out.push({(uint8_t)(ep_ + 7), (uint8_t)ep_, PROMO_NONE});
        }
    }
}

void BitboardPosition::gen_knights(MoveList& out) const {
    int ci = static_cast<int>(stm_);
    for (uint64_t b = pcs_[ci][2]; b;) {
        int from = pop_lsb(b);
        uint64_t moves = attacks::knight(from) & ~occ_[ci];
        while (moves) {
            int to = pop_lsb(moves);
            out.push({(uint8_t)from, (uint8_t)to, PROMO_NONE});
        }
    }
}

void BitboardPosition::gen_bishops(MoveList& out) const {
    int ci = static_cast<int>(stm_);
    for (uint64_t b = pcs_[ci][3]; b;) {
        int from = pop_lsb(b);
        uint64_t moves = attacks::bishop_otf(from, occ_all_) & ~occ_[ci];
        while (moves) {
            int to = pop_lsb(moves);
            out.push({(uint8_t)from, (uint8_t)to, PROMO_NONE});
        }
    }
}

void BitboardPosition::gen_rooks(MoveList& out) const {
    int ci = static_cast<int>(stm_);
    for (uint64_t b = pcs_[ci][5]; b;) {
        int from = pop_lsb(b);
        uint64_t moves = attacks::rook_otf(from, occ_all_) & ~occ_[ci];
        while (moves) {
            int to = pop_lsb(moves);
            out.push({(uint8_t)from, (uint8_t)to, PROMO_NONE});
        }
    }
}

void BitboardPosition::gen_queens(MoveList& out) const {
    int ci = static_cast<int>(stm_);
    for (uint64_t b = pcs_[ci][1]; b;) {
        int from = pop_lsb(b);
        uint64_t moves = (attacks::bishop_otf(from, occ_all_) |
                          attacks::rook_otf(from, occ_all_)) & ~occ_[ci];
        while (moves) {
            int to = pop_lsb(moves);
            out.push({(uint8_t)from, (uint8_t)to, PROMO_NONE});
        }
    }
}

void BitboardPosition::gen_king(MoveList& out) const {
    int ci  = static_cast<int>(stm_);
    Color them = other(stm_);
    int from = king_square(stm_);

    // Normal king moves
    uint64_t moves = attacks::king(from) & ~occ_[ci];
    while (moves) {
        int to = pop_lsb(moves);
        out.push({(uint8_t)from, (uint8_t)to, PROMO_NONE});
    }

    // Castling: check rights, path clear, and king not passing through check.
    // The legal filter will additionally verify the destination isn't attacked.
    if (stm_ == Color::White) {
        if ((cr_ & CR_WK) &&
            !(occ_all_ & ((1ULL << make_sq(5, 0)) | (1ULL << make_sq(6, 0)))) &&
            !square_attacked(make_sq(4, 0), them) &&
            !square_attacked(make_sq(5, 0), them)) {
            out.push({(uint8_t)make_sq(4, 0), (uint8_t)make_sq(6, 0), PROMO_NONE});
        }
        if ((cr_ & CR_WQ) &&
            !(occ_all_ & ((1ULL << make_sq(3, 0)) | (1ULL << make_sq(2, 0)) | (1ULL << make_sq(1, 0)))) &&
            !square_attacked(make_sq(4, 0), them) &&
            !square_attacked(make_sq(3, 0), them)) {
            out.push({(uint8_t)make_sq(4, 0), (uint8_t)make_sq(2, 0), PROMO_NONE});
        }
    } else {
        if ((cr_ & CR_BK) &&
            !(occ_all_ & ((1ULL << make_sq(5, 7)) | (1ULL << make_sq(6, 7)))) &&
            !square_attacked(make_sq(4, 7), them) &&
            !square_attacked(make_sq(5, 7), them)) {
            out.push({(uint8_t)make_sq(4, 7), (uint8_t)make_sq(6, 7), PROMO_NONE});
        }
        if ((cr_ & CR_BQ) &&
            !(occ_all_ & ((1ULL << make_sq(3, 7)) | (1ULL << make_sq(2, 7)) | (1ULL << make_sq(1, 7)))) &&
            !square_attacked(make_sq(4, 7), them) &&
            !square_attacked(make_sq(3, 7), them)) {
            out.push({(uint8_t)make_sq(4, 7), (uint8_t)make_sq(2, 7), PROMO_NONE});
        }
    }
}

void BitboardPosition::generate_pseudo(MoveList& out) const {
    out.clear();
    gen_pawns(out);
    gen_knights(out);
    gen_bishops(out);
    gen_rooks(out);
    gen_queens(out);
    gen_king(out);
}

void BitboardPosition::generate_legal(MoveList& out) {
    MoveList pseudo;
    generate_pseudo(pseudo);
    out.clear();

    for (int i = 0; i < pseudo.size; ++i) {
        Move m = pseudo.moves[i];
        do_move(m);
        // After do_move, stm_ is the opponent.
        // Check that the king of the side that just moved is not in check.
        bool legal = !square_attacked(king_square(other(stm_)), stm_);
        undo_move();
        if (legal) out.push(m);
    }
}

bool BitboardPosition::has_any_legal_move() {
    MoveList pseudo;
    generate_pseudo(pseudo);

    for (int i = 0; i < pseudo.size; ++i) {
        Move m = pseudo.moves[i];
        do_move(m);
        bool legal = !square_attacked(king_square(other(stm_)), stm_);
        undo_move();
        if (legal) return true;
    }
    return false;
}

} // namespace chess
