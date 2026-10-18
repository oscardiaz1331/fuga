#include "strategies.hpp"

namespace maze_gen
{

namespace
{

void divide(MazeGrid & grid, Random & rng, int x, int y, int width, int height)
{
  // A region one cell wide is already a corridor: nothing left to divide
  if (width < 2 || height < 2) {
    return;
  }

  // Cut across the longer side; pick at random when the region is square
  bool horizontal;
  if (width < height) {
    horizontal = true;
  } else if (height < width) {
    horizontal = false;
  } else {
    horizontal = rng.chance(0.5);
  }

  if (horizontal) {
    // Wall between rows wall_y - 1 and wall_y, with a single passage
    const int wall_y = y + rng.uniform_int(1, height - 1);
    const int passage_x = x + rng.uniform_int(0, width - 1);
    for (int cx = x; cx < x + width; ++cx) {
      if (cx != passage_x) {
        grid.add_wall(cx, wall_y - 1, Dir::S);
      }
    }
    divide(grid, rng, x, y, width, wall_y - y);
    divide(grid, rng, x, wall_y, width, y + height - wall_y);
  } else {
    // Wall between columns wall_x - 1 and wall_x, with a single passage
    const int wall_x = x + rng.uniform_int(1, width - 1);
    const int passage_y = y + rng.uniform_int(0, height - 1);
    for (int cy = y; cy < y + height; ++cy) {
      if (cy != passage_y) {
        grid.add_wall(wall_x - 1, cy, Dir::E);
      }
    }
    divide(grid, rng, x, y, wall_x - x, height);
    divide(grid, rng, wall_x, y, x + width - wall_x, height);
  }
}

}  // namespace

void RecursiveDivisionStrategy::generate(
  MazeGrid & grid, Random & rng, const MazeStrategyOptions &)
{
  // This algorithm adds walls instead of removing them, so it starts from one empty room
  for (int y = 0; y < grid.height(); ++y) {
    for (int x = 0; x < grid.width(); ++x) {
      grid.carve(x, y, Dir::E);
      grid.carve(x, y, Dir::S);
    }
  }
  divide(grid, rng, 0, 0, grid.width(), grid.height());
}

}  // namespace maze_gen
