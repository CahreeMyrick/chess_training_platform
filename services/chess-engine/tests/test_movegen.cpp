#include <gtest/gtest.h>
#include "test_helpers.hpp"
#include "movegen/movegen.hpp"
#include "core/movelist.hpp"

using namespace chess;
using namespace chess::test;

// Helper: count moves in MoveList that end on a given target square
static int count_moves_to(const MoveList& ml, int target) {
    int n = 0;
    for (int i = 0; i < ml.size; ++i) {
        if (ml.moves[i].to == target) ++n;
    }
    return n;
}

// --- Bulk legal move counts ---

TEST(MoveGenTest, StartposExactly20LegalMoves) {
    auto pos = startpos();
    MoveList ml;
    MoveGen::generate_legal(pos, ml);
    // 16 pawn moves (each of 8 pawns can go 1 or 2 squares) + 4 knight moves
    EXPECT_EQ(ml.size, 20);
}

TEST(MoveGenTest, HasLegalMoveFromStart) {
    auto pos = startpos();
    EXPECT_TRUE(MoveGen::has_any_legal_move(pos));
}

TEST(MoveGenTest, AfterE4BlackHas20LegalMoves) {
    auto pos = startpos();
    std::string err;
    pos.make_move(mv("e2", "e4"), err);
    MoveList ml;
    MoveGen::generate_legal(pos, ml);
    EXPECT_EQ(ml.size, 20);
}

// --- Per-piece move generation ---

TEST(MoveGenTest, KnightOnB1HasTwoMoves) {
    auto pos = startpos();
    MoveList ml;
    MoveGen::gen_knight(pos, sq("b1"), Piece::WN, ml);
    EXPECT_EQ(ml.size, 2); // a3 and c3
    EXPECT_EQ(count_moves_to(ml, sq("a3")), 1);
    EXPECT_EQ(count_moves_to(ml, sq("c3")), 1);
}

TEST(MoveGenTest, KnightOnG1HasTwoMoves) {
    auto pos = startpos();
    MoveList ml;
    MoveGen::gen_knight(pos, sq("g1"), Piece::WN, ml);
    EXPECT_EQ(ml.size, 2); // f3 and h3
    EXPECT_EQ(count_moves_to(ml, sq("f3")), 1);
    EXPECT_EQ(count_moves_to(ml, sq("h3")), 1);
}

TEST(MoveGenTest, KnightInCenterHasEightMoves) {
    auto pos = empty_pos();
    pos.set(sq("e1"), Piece::WK);
    pos.set(sq("e8"), Piece::BK);
    pos.set(sq("e4"), Piece::WN);

    MoveList ml;
    MoveGen::gen_knight(pos, sq("e4"), Piece::WN, ml);
    EXPECT_EQ(ml.size, 8);
}

TEST(MoveGenTest, KnightInCornerHasTwoMoves) {
    auto pos = empty_pos();
    pos.set(sq("e1"), Piece::WK);
    pos.set(sq("e8"), Piece::BK);
    pos.set(sq("a1"), Piece::WN);

    MoveList ml;
    MoveGen::gen_knight(pos, sq("a1"), Piece::WN, ml);
    EXPECT_EQ(ml.size, 2); // b3 and c2
}

TEST(MoveGenTest, PawnOnE2HasTwoMoves) {
    auto pos = startpos();
    MoveList ml;
    MoveGen::gen_pawn(pos, sq("e2"), Piece::WP, ml);
    EXPECT_EQ(ml.size, 2); // e3 and e4
    EXPECT_EQ(count_moves_to(ml, sq("e3")), 1);
    EXPECT_EQ(count_moves_to(ml, sq("e4")), 1);
}

TEST(MoveGenTest, BlackPawnOnE7HasTwoMoves) {
    auto pos = startpos();
    MoveList ml;
    MoveGen::gen_pawn(pos, sq("e7"), Piece::BP, ml);
    EXPECT_EQ(ml.size, 2); // e6 and e5
}

TEST(MoveGenTest, PawnBlockedHasNoMoves) {
    auto pos = empty_pos();
    pos.set(sq("e1"), Piece::WK);
    pos.set(sq("e8"), Piece::BK);
    pos.set(sq("e4"), Piece::WP);
    pos.set(sq("e5"), Piece::BP); // directly in front

    MoveList ml;
    MoveGen::gen_pawn(pos, sq("e4"), Piece::WP, ml);
    EXPECT_EQ(ml.size, 0);
}

TEST(MoveGenTest, PawnCanCaptureDiagonally) {
    auto pos = empty_pos();
    pos.set(sq("e1"), Piece::WK);
    pos.set(sq("e8"), Piece::BK);
    pos.set(sq("e4"), Piece::WP);
    pos.set(sq("d5"), Piece::BP); // capturable diagonal

    MoveList ml;
    MoveGen::gen_pawn(pos, sq("e4"), Piece::WP, ml);
    EXPECT_EQ(ml.size, 2); // e5 push + d5 capture
    EXPECT_EQ(count_moves_to(ml, sq("d5")), 1);
}

TEST(MoveGenTest, PawnPromotionGeneratesFourPieces) {
    auto pos = empty_pos();
    pos.set(sq("e1"), Piece::WK);
    pos.set(sq("a8"), Piece::BK); // keep BK off the e-file so e8 is empty
    pos.set(sq("e7"), Piece::WP); // one step from promotion

    MoveList ml;
    MoveGen::gen_pawn(pos, sq("e7"), Piece::WP, ml);
    EXPECT_EQ(ml.size, 4); // Q, R, B, N

    bool has[5] = {};
    for (int i = 0; i < ml.size; ++i) {
        has[ml.moves[i].promo] = true;
    }
    EXPECT_TRUE(has[PROMO_Q]);
    EXPECT_TRUE(has[PROMO_R]);
    EXPECT_TRUE(has[PROMO_B]);
    EXPECT_TRUE(has[PROMO_N]);
}

TEST(MoveGenTest, BishopOnD4Has13MovesOnEmptyBoard) {
    auto pos = empty_pos();
    pos.set(sq("e1"), Piece::WK);
    pos.set(sq("e8"), Piece::BK);
    pos.set(sq("d4"), Piece::WB);

    MoveList ml;
    MoveGen::gen_bishop(pos, sq("d4"), Piece::WB, ml);
    // NE:4  NW:3  SE:3  SW:3  (BK on e8 is not on any diagonal from d4)
    EXPECT_EQ(ml.size, 13);
}

TEST(MoveGenTest, RookOnD4Has14MovesOnEmptyBoard) {
    auto pos = empty_pos();
    pos.set(sq("e1"), Piece::WK);
    pos.set(sq("e8"), Piece::BK);
    pos.set(sq("d4"), Piece::WR);

    MoveList ml;
    MoveGen::gen_rook(pos, sq("d4"), Piece::WR, ml);
    // East:4  West:3  North:4  South:3
    EXPECT_EQ(ml.size, 14);
}

TEST(MoveGenTest, QueenOnD4Has27MovesOnEmptyBoard) {
    auto pos = empty_pos();
    pos.set(sq("e1"), Piece::WK);
    pos.set(sq("e8"), Piece::BK);
    pos.set(sq("d4"), Piece::WQ);

    MoveList ml;
    MoveGen::gen_queen(pos, sq("d4"), Piece::WQ, ml);
    // Rook-like:14  Bishop-like:13  (BK not on any ray from d4)
    EXPECT_EQ(ml.size, 27);
}

TEST(MoveGenTest, KingInCenterHasEightMoves) {
    auto pos = empty_pos();
    pos.set(sq("e4"), Piece::WK);
    pos.set(sq("e8"), Piece::BK);

    MoveList ml;
    MoveGen::gen_king(pos, sq("e4"), Piece::WK, ml);
    EXPECT_EQ(ml.size, 8);
}

// --- Legal move filtering ---

TEST(MoveGenTest, PinnedPieceCannotMove) {
    // White rook on e2 is pinned against White king on e1 by Black rook on e8
    auto pos = empty_pos();
    pos.set(sq("e1"), Piece::WK);
    pos.set(sq("e2"), Piece::WR); // pinned
    pos.set(sq("e8"), Piece::BK);
    pos.set(sq("e7"), Piece::BR); // pins the white rook

    MoveList ml;
    MoveGen::generate_legal(pos, ml);

    // The pinned WR should not be able to move off the e-file.
    // Verify no legal move moves the rook off the e-file.
    for (int i = 0; i < ml.size; ++i) {
        if (ml.moves[i].from == sq("e2")) {
            int dest_file = file_of(ml.moves[i].to);
            EXPECT_EQ(dest_file, 4) << "Pinned rook moved off e-file";
        }
    }
}
