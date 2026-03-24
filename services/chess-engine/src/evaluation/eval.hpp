#pragma once

#include "position/position.hpp"
#include "position/bb_position.hpp"

namespace chess {

class Eval {
public:
    static int evaluate(const Position& pos);
    static int evaluate(const BitboardPosition& pos);  // popcount-based, no square scan
};

} // namespace chess