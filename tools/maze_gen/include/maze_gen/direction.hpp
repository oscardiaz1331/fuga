#pragma once
#include <array>
#include <cstdint>

namespace maze_gen
{

// Walls sit between cells, one on each side. Every value is the bit of that wall in
// the wall mask of a cell: a set bit means "wall exists".
enum class Dir : uint8_t
{
  N = 1 << 0,  // North: y - 1
  E = 1 << 1,  // East:  x + 1
  S = 1 << 2,  // South: y + 1
  W = 1 << 3   // West:  x - 1
};

inline constexpr uint8_t kAllWalls = 0x0F;
inline constexpr std::array<Dir, 4> kDirections = {Dir::N, Dir::E, Dir::S, Dir::W};

constexpr Dir opposite(Dir d)
{
  switch (d) {
    case Dir::N: return Dir::S;
    case Dir::E: return Dir::W;
    case Dir::S: return Dir::N;
    case Dir::W: return Dir::E;
  }
  return Dir::N;  // unreachable
}

}  // namespace maze_gen
