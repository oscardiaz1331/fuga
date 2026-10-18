#include "box_drawing_chars.hpp"
#include <array>

namespace maze_gen
{

namespace
{

constexpr std::array<const char *, 16> kBox = {
  /* 0   0000 */ " ",   // no walls
  /* 1   0001 */ "╴",   // west only
  /* 2   0010 */ "╶",   // east only
  /* 3   0011 */ "─",   // east + west (horizontal)
  /* 4   0100 */ "╷",   // south only
  /* 5   0101 */ "┐",   // south + west
  /* 6   0110 */ "┌",   // south + east
  /* 7   0111 */ "┬",   // south + east + west (T pointing down)
  /* 8   1000 */ "╵",   // north only
  /* 9   1001 */ "┘",   // north + west
  /* 10  1010 */ "└",   // north + east
  /* 11  1011 */ "┴",   // north + east + west (T pointing up)
  /* 12  1100 */ "│",   // north + south (vertical)
  /* 13  1101 */ "┤",   // north + south + west
  /* 14  1110 */ "├",   // north + south + east
  /* 15  1111 */ "┼"    // north + south + east + west (cross)
};

}  // namespace

std::string_view get_box_drawing_char(bool n, bool s, bool e, bool w)
{
  unsigned index = (static_cast<unsigned>(n) << 3) |
    (static_cast<unsigned>(s) << 2) |
    (static_cast<unsigned>(e) << 1) |
    (static_cast<unsigned>(w) << 0);
  return kBox[index];
}

}  // namespace maze_gen
