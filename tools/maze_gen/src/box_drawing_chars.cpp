#include "maze_gen/box_drawing_chars.hpp"
#include <array>

namespace maze_gen {

static constexpr std::array<const char*, 16> BOX = {
    /* 0   0000 */ " ",   // no walls
    /* 1   0001 */ "╴",   // west only
    /* 2   0010 */ "╶",   // east only
    /* 3   0011 */ "─",   // east + west (horizontal)
    /* 4   0100 */ "╷",   // south only
    /* 5   0101 */ "┐",   // south + west (corner bottom-left open)
    /* 6   0110 */ "┌",   // south + east (corner bottom-right open)
    /* 7   0111 */ "┬",   // south + east + west (T pointing down)
    /* 8   1000 */ "╵",   // north only
    /* 9   1001 */ "┘",   // north + west (corner top-left open)
    /* 10  1010 */ "└",   // north + east (corner top-right open)
    /* 11  1011 */ "┴",   // north + east + west (T pointing up)
    /* 12  1100 */ "│",   // north + south (vertical)
    /* 13  1101 */ "┤",   // north + south + west (T pointing right)
    /* 14  1110 */ "├",   // north + south + east (T pointing left)
    /* 15  1111 */ "┼"    // north + south + east + west (cross)
};
std::string_view getBoxDrawingChar(bool n, bool s, bool e, bool w) {
    unsigned idx = (static_cast<unsigned>(n) << 3)
        | (static_cast<unsigned>(s) << 2)
        | (static_cast<unsigned>(e) << 1)
        | (static_cast<unsigned>(w) << 0);
    return BOX[idx];
}

}  // namespace maze_gen
