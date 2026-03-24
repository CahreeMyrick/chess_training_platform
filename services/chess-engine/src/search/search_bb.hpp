#pragma once

#include "core/move.hpp"
#include "position/bb_position.hpp"
#include "search/search.hpp"  // reuse SearchResult

namespace chess {

class SearchBB {
public:
    // Alpha-beta search using do_move / undo_move — no position copies.
    static SearchResult minimax(BitboardPosition& pos, int depth);
};

} // namespace chess
