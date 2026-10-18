#pragma once
#include "maze_gen/maze_grid.hpp"
#include "random.hpp"

namespace maze_gen
{

// Remove dead ends by opening one more wall in them, which turns a perfect maze into
// one with loops. fraction is the share of dead ends to remove: 0 leaves the maze as
// it is and 1 removes every dead end that has a wall left to open.
void braid(MazeGrid & grid, Random & rng, double fraction);

}  // namespace maze_gen
