#include <gtest/gtest.h>
#include "test_helpers.hpp"
#include "core/types.hpp"
#include "position/position.hpp"
#include "position/board_factory.hpp"

using namespace chess;
using namespace chess::test;

// --- Starting position layout ---

TEST(PositionTest, StartposWhiteBackRank) {
    auto pos = startpos();
    EXPECT_EQ(pos.at(make_sq(0, 0)), Piece::WR);
    EXPECT_EQ(pos.at(make_sq(1, 0)), Piece::WN);
    EXPECT_EQ(pos.at(make_sq(2, 0)), Piece::WB);
    EXPECT_EQ(pos.at(make_sq(3, 0)), Piece::WQ);
    EXPECT_EQ(pos.at(make_sq(4, 0)), Piece::WK);
    EXPECT_EQ(pos.at(make_sq(5, 0)), Piece::WB);
    EXPECT_EQ(pos.at(make_sq(6, 0)), Piece::WN);
    EXPECT_EQ(pos.at(make_sq(7, 0)), Piece::WR);
}

TEST(PositionTest, StartposBlackBackRank) {
    auto pos = startpos();
    EXPECT_EQ(pos.at(make_sq(0, 7)), Piece::BR);
    EXPECT_EQ(pos.at(make_sq(1, 7)), Piece::BN);
    EXPECT_EQ(pos.at(make_sq(2, 7)), Piece::BB);
    EXPECT_EQ(pos.at(make_sq(3, 7)), Piece::BQ);
    EXPECT_EQ(pos.at(make_sq(4, 7)), Piece::BK);
    EXPECT_EQ(pos.at(make_sq(5, 7)), Piece::BB);
    EXPECT_EQ(pos.at(make_sq(6, 7)), Piece::BN);
    EXPECT_EQ(pos.at(make_sq(7, 7)), Piece::BR);
}

TEST(PositionTest, StartposWhitePawns) {
    auto pos = startpos();
    for (int f = 0; f < 8; ++f) {
        EXPECT_EQ(pos.at(make_sq(f, 1)), Piece::WP) << "White pawn missing at file " << f;
    }
}

TEST(PositionTest, StartposBlackPawns) {
    auto pos = startpos();
    for (int f = 0; f < 8; ++f) {
        EXPECT_EQ(pos.at(make_sq(f, 6)), Piece::BP) << "Black pawn missing at file " << f;
    }
}

TEST(PositionTest, StartposMiddleEmpty) {
    auto pos = startpos();
    for (int r = 2; r <= 5; ++r) {
        for (int f = 0; f < 8; ++f) {
            EXPECT_EQ(pos.at(make_sq(f, r)), Piece::Empty)
                << "Expected empty at (" << f << "," << r << ")";
        }
    }
}

TEST(PositionTest, StartposSideToMove) {
    auto pos = startpos();
    EXPECT_EQ(pos.side_to_move(), Color::White);
}

TEST(PositionTest, StartposCastlingRights) {
    auto pos = startpos();
    EXPECT_EQ(pos.castling_rights(), CR_WK | CR_WQ | CR_BK | CR_BQ);
}

TEST(PositionTest, StartposNoEnPassant) {
    auto pos = startpos();
    EXPECT_EQ(pos.ep_square(), -1);
}

// --- make_move behavior ---

TEST(PositionTest, MakeMoveE2E4PawnMoves) {
    auto pos = startpos();
    std::string err;
    bool ok = pos.make_move(mv("e2", "e4"), err);
    ASSERT_TRUE(ok) << "e2-e4 rejected: " << err;
    EXPECT_EQ(pos.at(sq("e4")), Piece::WP);
    EXPECT_EQ(pos.at(sq("e2")), Piece::Empty);
}

TEST(PositionTest, MakeMoveTogglesSideToMove) {
    auto pos = startpos();
    std::string err;
    EXPECT_EQ(pos.side_to_move(), Color::White);
    pos.make_move(mv("e2", "e4"), err);
    EXPECT_EQ(pos.side_to_move(), Color::Black);
    pos.make_move(mv("e7", "e5"), err);
    EXPECT_EQ(pos.side_to_move(), Color::White);
}

TEST(PositionTest, DoublePawnPushSetsEnPassantSquare) {
    auto pos = startpos();
    std::string err;
    pos.make_move(mv("e2", "e4"), err);
    // After e2-e4 the en-passant target is e3 = make_sq(4,2)
    EXPECT_EQ(pos.ep_square(), make_sq(4, 2));
}

TEST(PositionTest, SinglePawnPushClearsEnPassant) {
    auto pos = startpos();
    std::string err;
    pos.make_move(mv("e2", "e3"), err);
    EXPECT_EQ(pos.ep_square(), -1);
}

TEST(PositionTest, MakeMoveRejectsOpponentPiece) {
    auto pos = startpos();
    std::string err;
    bool ok = pos.make_move(mv("e7", "e5"), err); // White to move but touching black pawn
    EXPECT_FALSE(ok);
    EXPECT_FALSE(err.empty());
}

TEST(PositionTest, MakeMoveRejectsFriendlyCapture) {
    auto pos = startpos();
    std::string err;
    // Try to move white rook onto white knight (a1 -> b1)
    bool ok = pos.make_move(mv("a1", "b1"), err);
    EXPECT_FALSE(ok);
}

TEST(PositionTest, CopyPositionIsIndependent) {
    auto pos1 = startpos();
    Position pos2 = pos1;  // copy

    std::string err;
    pos1.make_move(mv("e2", "e4"), err);

    // pos2 must be unchanged
    EXPECT_EQ(pos2.at(sq("e2")), Piece::WP);
    EXPECT_EQ(pos2.at(sq("e4")), Piece::Empty);
    EXPECT_EQ(pos2.side_to_move(), Color::White);

    // pos1 reflects the move
    EXPECT_EQ(pos1.at(sq("e4")), Piece::WP);
    EXPECT_EQ(pos1.at(sq("e2")), Piece::Empty);
    EXPECT_EQ(pos1.side_to_move(), Color::Black);
}

TEST(PositionTest, KingMoveClearsCastlingRights) {
    auto pos = empty_pos(Color::White);
    pos.set(sq("e1"), Piece::WK);
    pos.set(sq("h1"), Piece::WR);
    pos.set(sq("e8"), Piece::BK);
    pos.set_castling_rights(CR_WK | CR_WQ);

    std::string err;
    bool ok = pos.make_move(mv("e1", "f1"), err);
    ASSERT_TRUE(ok) << err;
    EXPECT_EQ(pos.castling_rights() & (CR_WK | CR_WQ), 0u)
        << "White castling rights must be cleared after king moves";
}

TEST(PositionTest, RookMoveClears_ItsOwnCastlingSide) {
    auto pos = empty_pos(Color::White);
    pos.set(sq("e1"), Piece::WK);
    pos.set(sq("a1"), Piece::WR);
    pos.set(sq("h1"), Piece::WR);
    pos.set(sq("e8"), Piece::BK);
    pos.set_castling_rights(CR_WK | CR_WQ);

    std::string err;
    // Move the queenside rook — should only lose CR_WQ
    pos.make_move(mv("a1", "a2"), err);
    EXPECT_EQ(pos.castling_rights() & CR_WQ, 0u);
    EXPECT_NE(pos.castling_rights() & CR_WK, 0u);
}

// --- Evaluation sanity ---

TEST(PositionTest, EvalStartposIsZero) {
    auto pos = startpos();
    // Symmetric starting position: evaluation must be 0
    int score = 0;
    for (int s = 0; s < 64; ++s) {
        Piece p = pos.at(s);
        switch (p) {
            case Piece::WP: score += 100;  break;
            case Piece::WN: score += 320;  break;
            case Piece::WB: score += 330;  break;
            case Piece::WR: score += 500;  break;
            case Piece::WQ: score += 900;  break;
            case Piece::BP: score -= 100;  break;
            case Piece::BN: score -= 320;  break;
            case Piece::BB: score -= 330;  break;
            case Piece::BR: score -= 500;  break;
            case Piece::BQ: score -= 900;  break;
            default: break;
        }
    }
    EXPECT_EQ(score, 0);
}
