#include "dead_end_braider.hpp"
#include <utility>
#include <vector>

namespace maze_gen
{

namespace
{

int open_sides(const MazeGrid & grid, int x, int y)
{
  int count = 0;
  for (const auto & [nx, ny, dir] : grid.neighbors(x, y)) {
    if (!grid.has_wall(x, y, dir)) {
      ++count;
    }
  }
  return count;
}

}  // namespace

void braid(MazeGrid & grid, Random & rng, double fraction)
{
  std::vector<std::pair<int, int>> dead_ends;
  for (int y = 0; y < grid.height(); ++y) {
    for (int x = 0; x < grid.width(); ++x) {
      if (open_sides(grid, x, y) == 1) {
        dead_ends.push_back({x, y});
      }
    }
  }
  rng.shuffle(dead_ends.begin(), dead_ends.end());

  for (const auto & [x, y] : dead_ends) {
    // The draw comes first so that it is made the same number of times whatever the maze
    const bool remove = rng.chance(fraction);
    if (!remove || open_sides(grid, x, y) != 1) {
      continue;  // kept, or already opened from the other side by an earlier one
    }

    // Opening towards another dead end removes two of them with a single wall
    NeighborList walled;
    NeighborList walled_dead_ends;
    for (const Neighbor & candidate : grid.neighbors(x, y)) {
      if (!grid.has_wall(x, y, candidate.dir)) {
        continue;
      }
      walled.push_back(candidate);
      if (open_sides(grid, candidate.x, candidate.y) == 1) {
        walled_dead_ends.push_back(candidate);
      }
    }
    const NeighborList & choices = walled_dead_ends.empty() ? walled : walled_dead_ends;
    if (choices.empty()) {
      continue;  // the end of a corridor one cell wide: there is no other wall to open
    }
    grid.carve(x, y, choices[rng.index(choices.size())].dir);
  }
}

}  // namespace maze_gen
