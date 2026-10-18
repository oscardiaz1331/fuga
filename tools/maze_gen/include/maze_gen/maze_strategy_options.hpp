#pragma once
#include <cstdint>
#include <optional>
#include <vector>
#include "maze_gen/maze_grid.hpp"

namespace maze_gen
{

// What MazeGenerator::generate should build. A value outside its range is rejected
// with std::invalid_argument; nothing is clamped or ignored silently.
struct MazeStrategyOptions
{
  // 0 to 10: how many dead ends are left in the maze. 10 keeps them all, which gives
  // a perfect maze (a single path between any two cells); 0 removes them all, which
  // gives loops and alternative routes everywhere. The cave generator ignores it.
  int difficulty = 10;

  // Same seed, same maze. Without a seed every maze is different
  std::optional<uint64_t> seed;

  // Exits opened at random on the border, never on a corner cell
  int num_random_exits = 1;
  // Exits opened exactly here. Each one has to be on a border cell and point outside
  std::vector<Exit> fixed_exits;

  // Open rectangular rooms inside the maze. The cave generator never adds them
  bool rooms = true;
  // 0 to 10: how many rooms and how large
  int room_level = 0;

  // Growing Tree, 0 to 1: how often the next cell to grow from is the newest one
  // instead of a random one. 0 behaves like Prim, 1 like DFS
  double growing_tree_newest_ratio = 0.0;
  // Sidewinder, 0 to 1: chance of extending a run to the east instead of closing it
  double sidewinder_east_probability = 0.5;
  // Eller, 0 to 1: chance of joining two neighboring cells of different sets
  double ellers_join_probability = 0.5;
  // Cave generator, 0 to 10: how much of the cave is solid rock, from about a sixth
  // of the cells to about half of them
  int cave_level = 5;
};

}  // namespace maze_gen
