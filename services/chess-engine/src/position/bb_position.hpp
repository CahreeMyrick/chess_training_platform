#pragma once

#include <cstdint>
#include <vector>
#include "core/types.hpp"
#include "core/move.hpp"
#include "core/movelist.hpp"

namespace chess {

// BitboardPosition — a self-contained, bitboard-native position type.
//
// Differences from the existing Position class:
//   - Uses do_move / undo_move (undo stack) instead of full position copies.
//   - Bitboard-native move generation (whole-board shifts for pawns, attack
//     table lookups for knight/king, OTF ray casts for sliders).
//   - square_attacked() uses pure bitboard intersection — no square scanning.
//
// This class does NOT inherit from Board/Position and does not support the
// polymorphic board-type switching. It is the dedicated fast path.

class BitboardPosition {
public:
    BitboardPosition();
    static BitboardPosition startpos();

    // --- Board state accessors ---
    Piece     piece_at(int sq) const { return mailbox_[sq]; }
    Color     side_to_move()   const { return stm_; }
    uint8_t   castling_rights() const { return cr_; }
    int       ep_square()      const { return ep_; }

    // Raw bitboard access for external consumers (e.g. eval).
    // Piece-type indices within a color: 0=K 1=Q 2=N 3=B 4=P 5=R
    uint64_t pieces(Color c, int pt_idx) const { return pcs_[static_cast<int>(c)][pt_idx]; }
    uint64_t occ_all() const { return occ_all_; }
    uint64_t occ(Color c) const { return occ_[static_cast<int>(c)]; }

    // --- King location ---
    int king_square(Color c) const;

    // --- Attack detection (pure bitboard) ---
    bool square_attacked(int sq, Color by) const;
    bool in_check(Color who) const;

    // --- Move execution (undo stack — O(1) per make/unmake) ---
    void do_move(Move m);
    void undo_move();

    // --- Move generation ---
    void generate_pseudo(MoveList& out) const;
    void generate_legal(MoveList& out);
    bool has_any_legal_move();

private:
    // 12 piece bitboards: pcs_[color][piece_type_index]
    // Piece type indices: 0=King 1=Queen 2=Knight 3=Bishop 4=Pawn 5=Rook
    uint64_t pcs_[2][6] = {};

    // Layered occupancy (kept in sync with piece bitboards)
    uint64_t occ_[2]  = {};
    uint64_t occ_all_ = 0;

    // Mailbox for O(1) piece-at-square lookup (used in do_move, undo_move)
    Piece mailbox_[64] = {};

    // Game state
    Color   stm_ = Color::White;
    uint8_t cr_  = CR_NONE;
    int     ep_  = -1;

    // Undo stack entry
    struct State {
        uint8_t cr;
        int8_t  ep;
        Piece   captured;  // Piece::Empty if no capture
        Piece   moving;    // piece that moved (pawn, not promotion piece)
        bool    was_ep;    // true if this was an en-passant capture
        Move    m;         // the move itself (needed for undo logic)
    };
    std::vector<State> stack_;

    // --- Internal bitboard primitives ---
    static int piece_ci(Piece p)  { return (static_cast<int>(p) - 1) / 6; }
    static int piece_ti(Piece p)  { return (static_cast<int>(p) - 1) % 6; }
    static Piece promo_piece(Color c, Promotion pr);

    void put_piece(Piece p, int sq);
    void remove_piece(Piece p, int sq);
    void move_piece(Piece p, int from, int to);

    // --- Internal movegen helpers ---
    void gen_pawns(MoveList& out) const;
    void gen_knights(MoveList& out) const;
    void gen_bishops(MoveList& out) const;
    void gen_rooks(MoveList& out) const;
    void gen_queens(MoveList& out) const;
    void gen_king(MoveList& out) const;
};

} // namespace chess
