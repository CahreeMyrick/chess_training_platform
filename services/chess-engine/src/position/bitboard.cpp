#include "position/bitboard.hpp"

namespace chess {

Bitboard::Bitboard() {
    for (int i = 0; i < 64; ++i) mailbox_[i] = Piece::Empty;
}

uint64_t Bitboard::bit(int sq) {
    return 1ULL << sq;
}

void Bitboard::clear_all_at(int sq) {
    Piece existing = mailbox_[sq];
    if (existing == Piece::Empty) return;

    uint64_t mask = ~bit(sq);

    // Clear the specific piece bitboard
    switch (existing) {
        case Piece::WP: wp_ &= mask; break;
        case Piece::WN: wn_ &= mask; break;
        case Piece::WB: wb_ &= mask; break;
        case Piece::WR: wr_ &= mask; break;
        case Piece::WQ: wq_ &= mask; break;
        case Piece::WK: wk_ &= mask; break;
        case Piece::BP: bp_ &= mask; break;
        case Piece::BN: bn_ &= mask; break;
        case Piece::BB: bb_ &= mask; break;
        case Piece::BR: br_ &= mask; break;
        case Piece::BQ: bq_ &= mask; break;
        case Piece::BK: bk_ &= mask; break;
        default: break;
    }

    // Update occupancy
    int ci = is_white(existing) ? 0 : 1;
    occ_[ci]  &= mask;
    occ_all_  &= mask;

    mailbox_[sq] = Piece::Empty;
}

Piece Bitboard::piece_at(int sq) const {
    return mailbox_[sq];
}

void Bitboard::set_piece(int sq, Piece p) {
    clear_all_at(sq);
    if (p == Piece::Empty) return;

    uint64_t b = bit(sq);

    switch (p) {
        case Piece::WP: wp_ |= b; break;
        case Piece::WN: wn_ |= b; break;
        case Piece::WB: wb_ |= b; break;
        case Piece::WR: wr_ |= b; break;
        case Piece::WQ: wq_ |= b; break;
        case Piece::WK: wk_ |= b; break;
        case Piece::BP: bp_ |= b; break;
        case Piece::BN: bn_ |= b; break;
        case Piece::BB: bb_ |= b; break;
        case Piece::BR: br_ |= b; break;
        case Piece::BQ: bq_ |= b; break;
        case Piece::BK: bk_ |= b; break;
        default: break;
    }

    int ci = is_white(p) ? 0 : 1;
    occ_[ci]  |= b;
    occ_all_  |= b;

    mailbox_[sq] = p;
}

void Bitboard::clear_square(int sq) {
    clear_all_at(sq);
}

std::unique_ptr<Board> Bitboard::clone() const {
    return std::make_unique<Bitboard>(*this);
}

} // namespace chess
