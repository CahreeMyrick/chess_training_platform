#pragma once

#include <memory>
#include "position/board.hpp"

namespace chess {

enum class BoardType {
    Array,
    Pointer,
    Bitboard
};

/**
 * @brief Returns a new instance of a Board implementation based on the specified type.
 * @param type The type of board to create (Array, Pointer, Bitboard).
 * @return A unique pointer to the created Board instance. Returns nullptr if the type is unrecognized.
 */
std::unique_ptr<Board> make_board(BoardType type);

} // namespace chess
