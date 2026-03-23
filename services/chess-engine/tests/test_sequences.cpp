// test_sequences.cpp
//
// Full-game sequence tests.  Run with --visualize to see the board evolve
// after every move directly in the terminal, e.g.:
//
//   ./chess_tests --visualize
//   ./chess_tests --visualize --gtest_filter="*FoolsMate*"
//

#include <gtest/gtest.h>
#include "test_helpers.hpp"
#include "rules/rules.hpp"
#include "movegen/movegen.hpp"

using namespace chess;
using namespace chess::test;

// ---------------------------------------------------------------------------
// Fool's Mate — fastest checkmate in chess (Black wins in 2)
// 1.f3 e5  2.g4?? Qh4#
// ---------------------------------------------------------------------------
TEST(SequencesTest, FoolsMate) {
    auto pos = startpos();
    show(pos, "Fool's Mate — starting position");

    const std::vector<Move> moves = {
        mv("f2", "f3"),  // 1. f3  — weakens the king
        mv("e7", "e5"),  // 1... e5
        mv("g2", "g4"),  // 2. g4?? — fatal
        mv("d8", "h4"),  // 2... Qh4#
    };
    const std::vector<std::string> labels = {
        "1. f2-f3  — White weakens the kingside",
        "1... e7-e5 — Black occupies the centre",
        "2. g2-g4?? — Blunder! Opens the diagonal",
        "2... Qd8-h4# — Checkmate!",
    };

    ASSERT_TRUE(play_moves(pos, moves, labels)) << "Sequence failed unexpectedly";
    show(pos, "Final: Fool's Mate");

    EXPECT_TRUE (Rules::in_check(pos, Color::White)) << "White should be in check";
    EXPECT_FALSE(MoveGen::has_any_legal_move(pos))   << "White should have no legal moves";

    if (g_visualize) {
        std::cout << "\033[1;31m*** Fool's Mate — White is checkmated! ***\033[0m\n\n";
    }
}

// ---------------------------------------------------------------------------
// Scholar's Mate — 4-move checkmate against Black
// 1.e4 e5  2.Bc4 Nc6  3.Qh5 Nf6??  4.Qxf7#
// ---------------------------------------------------------------------------
TEST(SequencesTest, ScholarsMate) {
    auto pos = startpos();
    show(pos, "Scholar's Mate — starting position");

    const std::vector<Move> moves = {
        mv("e2", "e4"),  // 1. e4
        mv("e7", "e5"),  // 1... e5
        mv("f1", "c4"),  // 2. Bc4
        mv("b8", "c6"),  // 2... Nc6
        mv("d1", "h5"),  // 3. Qh5 — threatens Qxf7#
        mv("g8", "f6"),  // 3... Nf6?? — misses the threat
        mv("h5", "f7"),  // 4. Qxf7#
    };
    const std::vector<std::string> labels = {
        "1.  e2-e4   — King's pawn",
        "1... e7-e5  — Mirror reply",
        "2.  Bf1-c4  — Italian bishop, eyes f7",
        "2... Nb8-c6 — Knight development",
        "3.  Qd1-h5  — Queen to h5, double threat",
        "3... Ng8-f6?? — Blunder! Ignores Qxf7",
        "4.  Qh5xf7# — Scholar's Mate!",
    };

    ASSERT_TRUE(play_moves(pos, moves, labels)) << "Sequence failed unexpectedly";
    show(pos, "Final: Scholar's Mate");

    EXPECT_TRUE (Rules::in_check(pos, Color::Black)) << "Black should be in check";
    EXPECT_FALSE(MoveGen::has_any_legal_move(pos))   << "Black should have no legal moves";

    if (g_visualize) {
        std::cout << "\033[1;31m*** Scholar's Mate — Black is checkmated! ***\033[0m\n\n";
    }
}

// ---------------------------------------------------------------------------
// Italian Game opening — 5-move development sequence
// 1.e4 e5  2.Nf3 Nc6  3.Bc4
// ---------------------------------------------------------------------------
TEST(SequencesTest, ItalianGameOpening) {
    auto pos = startpos();
    show(pos, "Italian Game — starting position");

    const std::vector<Move> moves = {
        mv("e2", "e4"),  // 1. e4
        mv("e7", "e5"),  // 1... e5
        mv("g1", "f3"),  // 2. Nf3
        mv("b8", "c6"),  // 2... Nc6
        mv("f1", "c4"),  // 3. Bc4  — Italian Game
    };
    const std::vector<std::string> labels = {
        "1.  e2-e4   — King's pawn opening",
        "1... e7-e5  — Symmetrical reply",
        "2.  Ng1-f3  — Develop knight, pressure e5",
        "2... Nb8-c6 — Defend the e5 pawn",
        "3.  Bf1-c4  — Italian Game! Bishop targets f7",
    };

    ASSERT_TRUE(play_moves(pos, moves, labels)) << "Opening sequence failed";
    show(pos, "Italian Game after 3.Bc4");

    // Verify pieces are on the expected squares
    EXPECT_EQ(pos.at(sq("e4")), Piece::WP);
    EXPECT_EQ(pos.at(sq("e5")), Piece::BP);
    EXPECT_EQ(pos.at(sq("f3")), Piece::WN);
    EXPECT_EQ(pos.at(sq("c6")), Piece::BN);
    EXPECT_EQ(pos.at(sq("c4")), Piece::WB);

    // Original squares now empty
    EXPECT_EQ(pos.at(sq("e2")), Piece::Empty);
    EXPECT_EQ(pos.at(sq("g1")), Piece::Empty);
    EXPECT_EQ(pos.at(sq("f1")), Piece::Empty);

    EXPECT_EQ(pos.side_to_move(), Color::Black);
    EXPECT_FALSE(Rules::in_check(pos, Color::White));
    EXPECT_FALSE(Rules::in_check(pos, Color::Black));
}

// ---------------------------------------------------------------------------
// Kingside castling
// ---------------------------------------------------------------------------
TEST(SequencesTest, KingsideCastling) {
    // Clear the f1/g1 squares manually so the king can castle immediately
    auto pos = startpos();
    pos.clear(sq("f1"));
    pos.clear(sq("g1"));
    show(pos, "Before castling (f1 and g1 cleared)");

    std::string err;
    bool ok = pos.make_move(mv("e1", "g1"), err);
    ASSERT_TRUE(ok) << "Castling rejected: " << err;
    show(pos, "After O-O (kingside castle)");

    EXPECT_EQ(pos.at(sq("g1")), Piece::WK) << "King must land on g1";
    EXPECT_EQ(pos.at(sq("f1")), Piece::WR) << "Rook must land on f1";
    EXPECT_EQ(pos.at(sq("e1")), Piece::Empty);
    EXPECT_EQ(pos.at(sq("h1")), Piece::Empty);
    EXPECT_EQ(pos.castling_rights() & (CR_WK | CR_WQ), 0u);
}

// ---------------------------------------------------------------------------
// Queenside castling
// ---------------------------------------------------------------------------
TEST(SequencesTest, QueensideCastling) {
    auto pos = startpos();
    pos.clear(sq("b1"));
    pos.clear(sq("c1"));
    pos.clear(sq("d1"));
    show(pos, "Before O-O-O (b1/c1/d1 cleared)");

    std::string err;
    bool ok = pos.make_move(mv("e1", "c1"), err);
    ASSERT_TRUE(ok) << "Queenside castling rejected: " << err;
    show(pos, "After O-O-O (queenside castle)");

    EXPECT_EQ(pos.at(sq("c1")), Piece::WK) << "King must land on c1";
    EXPECT_EQ(pos.at(sq("d1")), Piece::WR) << "Rook must land on d1";
    EXPECT_EQ(pos.at(sq("e1")), Piece::Empty);
    EXPECT_EQ(pos.at(sq("a1")), Piece::Empty);
    EXPECT_EQ(pos.castling_rights() & (CR_WK | CR_WQ), 0u);
}

// ---------------------------------------------------------------------------
// En passant capture
// 1.e4 Nf6  2.e5 d5  3.exd6 e.p.
// ---------------------------------------------------------------------------
TEST(SequencesTest, EnPassantCapture) {
    auto pos = startpos();
    std::string err;

    pos.make_move(mv("e2", "e4"), err);           // 1. e4
    pos.make_move(mv("g8", "f6"), err);           // 1... Nf6
    pos.make_move(mv("e4", "e5"), err);           // 2. e5
    show(pos, "After 2.e5 — about to play d7-d5");

    pos.make_move(mv("d7", "d5"), err);           // 2... d5
    show(pos, "After 2...d5 — en passant available on d6");

    EXPECT_EQ(pos.ep_square(), sq("d6")) << "EP target square must be d6";

    bool ok = pos.make_move(mv("e5", "d6"), err); // 3. exd6 e.p.
    ASSERT_TRUE(ok) << "En passant failed: " << err;
    show(pos, "After 3.exd6 (en passant)");

    EXPECT_EQ(pos.at(sq("d6")), Piece::WP)    << "White pawn should be on d6";
    EXPECT_EQ(pos.at(sq("d5")), Piece::Empty) << "Captured pawn must be removed from d5";
    EXPECT_EQ(pos.at(sq("e5")), Piece::Empty) << "e5 must be empty";

    if (g_visualize) {
        std::cout << "\033[1;32m*** En passant successfully executed! ***\033[0m\n\n";
    }
}

// ---------------------------------------------------------------------------
// Pawn promotion to queen
// ---------------------------------------------------------------------------
TEST(SequencesTest, PawnPromotionToQueen) {
    auto pos = empty_pos(Color::White);
    pos.set(sq("e1"), Piece::WK);
    pos.set(sq("a8"), Piece::BK); // keep BK off e8 so the pawn can promote there
    pos.set(sq("e7"), Piece::WP);
    show(pos, "Pawn on e7, about to promote");

    std::string err;
    bool ok = pos.make_move(Move{(uint8_t)sq("e7"), (uint8_t)sq("e8"), PROMO_Q}, err);
    ASSERT_TRUE(ok) << "Promotion failed: " << err;
    show(pos, "After e7-e8=Q");

    EXPECT_EQ(pos.at(sq("e8")), Piece::WQ) << "Promoted piece must be a white queen";
    EXPECT_EQ(pos.at(sq("e7")), Piece::Empty);
}

// ---------------------------------------------------------------------------
// Stalemate — no legal moves but king not in check
// White Kf6 + Qg6, Black Kh8, Black to move
// ---------------------------------------------------------------------------
TEST(SequencesTest, Stalemate) {
    auto pos = empty_pos(Color::Black);
    pos.set(sq("f6"), Piece::WK);
    pos.set(sq("g6"), Piece::WQ);
    pos.set(sq("h8"), Piece::BK);
    show(pos, "Stalemate position — Black to move");

    EXPECT_FALSE(Rules::in_check(pos, Color::Black))
        << "Black king must NOT be in check in stalemate";
    EXPECT_FALSE(MoveGen::has_any_legal_move(pos))
        << "Black must have no legal moves in stalemate";

    if (g_visualize) {
        std::cout << "\033[1;33m*** Stalemate! Black king is trapped but not in check. ***\033[0m\n\n";
    }
}
