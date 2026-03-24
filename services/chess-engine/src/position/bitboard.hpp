#pragma once

#include <cstdint>
#include <memory>
#include "position/board.hpp"

namespace chess {

class Bitboard : public Board {
public:
    Bitboard();

    Piece piece_at(int sq) const override;
    void set_piece(int sq, Piece p) override;
    void clear_square(int sq) override;

    std::unique_ptr<Board> clone() const override;

    // Fast occupancy access — available to callers that downcast to Bitboard.
    uint64_t occ_all() const { return occ_all_; }
    uint64_t occ(Color c)  const { return occ_[static_cast<int>(c)]; }

private:
    // 12 per-piece bitboards
    uint64_t wp_ = 0, wn_ = 0, wb_ = 0, wr_ = 0, wq_ = 0, wk_ = 0;
    uint64_t bp_ = 0, bn_ = 0, bb_ = 0, br_ = 0, bq_ = 0, bk_ = 0;

    // Occupancy bitboards kept in sync with piece bitboards
    uint64_t occ_[2]  = {};   // [0]=White, [1]=Black
    uint64_t occ_all_ = 0;

    // Mailbox: O(1) piece lookup by square index
    Piece mailbox_[64] = {};

    static uint64_t bit(int sq);
    void clear_all_at(int sq);
    void set_occ_for(int sq, Piece p);
};

} // namespace chess
