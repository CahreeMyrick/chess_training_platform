#include <gtest/gtest.h>
#include "test_helpers.hpp"
#include "rules/rules.hpp"
#include "movegen/movegen.hpp"
#include "evaluation/eval.hpp"

using namespace chess;
using namespace chess::test;

// --- in_check ---

TEST(RulesTest, NotInCheckAtStart) {
    auto pos = startpos();
    EXPECT_FALSE(Rules::in_check(pos, Color::White));
    EXPECT_FALSE(Rules::in_check(pos, Color::Black));
}

TEST(RulesTest, KingInCheckFromRookOnSameFile) {
    auto pos = empty_pos();
    pos.set(sq("e1"), Piece::WK);
    pos.set(sq("e5"), Piece::BR); // Black rook on e-file
    pos.set(sq("e8"), Piece::BK);

    EXPECT_TRUE(Rules::in_check(pos, Color::White));
    EXPECT_FALSE(Rules::in_check(pos, Color::Black));
}

TEST(RulesTest, KingInCheckFromRookOnSameRank) {
    auto pos = empty_pos();
    pos.set(sq("e1"), Piece::WK);
    pos.set(sq("a1"), Piece::BR); // Black rook on rank 1
    pos.set(sq("e8"), Piece::BK);

    // a1 and e1 are on the same rank — but white pawns block in startpos.
    // Here the board is empty so the rook has a clear line.
    EXPECT_TRUE(Rules::in_check(pos, Color::White));
}

TEST(RulesTest, KingInCheckFromBishopOnDiagonal) {
    // WK on e1=(4,0), BB on h4=(7,3): delta=(3,3) -> diagonal
    auto pos = empty_pos();
    pos.set(sq("e1"), Piece::WK);
    pos.set(sq("h4"), Piece::BB);
    pos.set(sq("e8"), Piece::BK);

    EXPECT_TRUE(Rules::in_check(pos, Color::White));
}

TEST(RulesTest, KingInCheckFromKnight) {
    // WK on e4=(4,3), BN on f6=(5,5): delta=(1,2) -> knight jump
    auto pos = empty_pos();
    pos.set(sq("e4"), Piece::WK);
    pos.set(sq("f6"), Piece::BN);
    pos.set(sq("e8"), Piece::BK);

    EXPECT_TRUE(Rules::in_check(pos, Color::White));
}

TEST(RulesTest, KingInCheckFromPawn) {
    // WK on e4=(4,3), BP on d5=(3,4): BP attacks e4 diagonally (black pawn attacks down)
    auto pos = empty_pos();
    pos.set(sq("e4"), Piece::WK);
    pos.set(sq("d5"), Piece::BP);
    pos.set(sq("e8"), Piece::BK);

    EXPECT_TRUE(Rules::in_check(pos, Color::White));
}

TEST(RulesTest, CheckBlockedByIntermediatePiece) {
    auto pos = empty_pos();
    pos.set(sq("e1"), Piece::WK);
    pos.set(sq("e5"), Piece::BR); // would give check...
    pos.set(sq("e3"), Piece::WP); // ...but blocked by White pawn
    pos.set(sq("e8"), Piece::BK);

    EXPECT_FALSE(Rules::in_check(pos, Color::White));
}

// --- find_king ---

TEST(RulesTest, FindKingAtStartpos) {
    auto pos = startpos();
    EXPECT_EQ(Rules::find_king(pos, Color::White), sq("e1"));
    EXPECT_EQ(Rules::find_king(pos, Color::Black), sq("e8"));
}

// --- is_square_attacked ---

TEST(RulesTest, SquareAttackedByRook) {
    auto pos = empty_pos();
    pos.set(sq("e1"), Piece::WK);
    pos.set(sq("e8"), Piece::BK);
    pos.set(sq("d1"), Piece::WR); // White rook on d1

    EXPECT_TRUE(Rules::is_square_attacked(pos, sq("d5"), Color::White));
    EXPECT_TRUE(Rules::is_square_attacked(pos, sq("a1"), Color::White));
    EXPECT_FALSE(Rules::is_square_attacked(pos, sq("e5"), Color::White));
}

TEST(RulesTest, SquareAttackedByPawn) {
    auto pos = empty_pos();
    pos.set(sq("e1"), Piece::WK);
    pos.set(sq("e8"), Piece::BK);
    pos.set(sq("d5"), Piece::WP); // WP on d5 attacks c6 and e6

    EXPECT_TRUE(Rules::is_square_attacked(pos, sq("c6"), Color::White));
    EXPECT_TRUE(Rules::is_square_attacked(pos, sq("e6"), Color::White));
    EXPECT_FALSE(Rules::is_square_attacked(pos, sq("d6"), Color::White)); // straight ahead is not attack
}

// --- castling ---

TEST(RulesTest, IsCastleMoveKingside) {
    auto pos = empty_pos();
    pos.set(sq("e1"), Piece::WK);
    pos.set(sq("h1"), Piece::WR);
    pos.set(sq("e8"), Piece::BK);
    pos.set_castling_rights(CR_WK);

    EXPECT_TRUE(Rules::is_castle_move(pos, mv("e1", "g1")));
}

TEST(RulesTest, IsCastleMoveQueenside) {
    auto pos = empty_pos();
    pos.set(sq("e1"), Piece::WK);
    pos.set(sq("a1"), Piece::WR);
    pos.set(sq("e8"), Piece::BK);
    pos.set_castling_rights(CR_WQ);

    EXPECT_TRUE(Rules::is_castle_move(pos, mv("e1", "c1")));
}

TEST(RulesTest, CastlePathSafeWhenClear) {
    auto pos = empty_pos();
    pos.set(sq("e1"), Piece::WK);
    pos.set(sq("h1"), Piece::WR);
    pos.set(sq("e8"), Piece::BK);
    pos.set_castling_rights(CR_WK);

    std::string err;
    EXPECT_TRUE(Rules::castle_path_safe(pos, mv("e1", "g1"), err));
}

TEST(RulesTest, CastlePathNotSafeWhenKingPassesThroughCheck) {
    auto pos = empty_pos();
    pos.set(sq("e1"), Piece::WK);
    pos.set(sq("h1"), Piece::WR);
    pos.set(sq("e8"), Piece::BK);
    pos.set(sq("f8"), Piece::BR); // Black rook attacks f1 (king passes through it)
    pos.set_castling_rights(CR_WK);

    std::string err;
    EXPECT_FALSE(Rules::castle_path_safe(pos, mv("e1", "g1"), err));
}

// --- Evaluation ---

TEST(EvalTest, StartposScoreIsZero) {
    auto pos = startpos();
    EXPECT_EQ(Eval::evaluate(pos), 0);
}

TEST(EvalTest, ExtraPawnIsPositiveForWhite) {
    auto pos = empty_pos();
    pos.set(sq("e1"), Piece::WK);
    pos.set(sq("e8"), Piece::BK);
    pos.set(sq("d4"), Piece::WP);

    EXPECT_GT(Eval::evaluate(pos), 0);
}

TEST(EvalTest, ExtraQueenIsNegativeForBlack) {
    auto pos = empty_pos();
    pos.set(sq("e1"), Piece::WK);
    pos.set(sq("e8"), Piece::BK);
    pos.set(sq("d5"), Piece::BQ);

    EXPECT_LT(Eval::evaluate(pos), 0);
}

TEST(EvalTest, MaterialBalanceReflectsPieceValues) {
    auto pos = empty_pos();
    pos.set(sq("e1"), Piece::WK);
    pos.set(sq("e8"), Piece::BK);
    pos.set(sq("d4"), Piece::WR); // +500
    pos.set(sq("d5"), Piece::BN); // -320
    // net = +180

    EXPECT_EQ(Eval::evaluate(pos), 500 - 320);
}
