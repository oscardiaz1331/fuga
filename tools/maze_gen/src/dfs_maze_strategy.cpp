#include <utility>
#include <vector>
#include "strategies.hpp"

namespace maze_gen
{

void DFSMazeStrategy::generate(MazeGrid & grid, Random & rng, const MazeStrategyOptions &)
{
  std::vector<uint8_t> visited(grid.cell_count(), 0);
  // An explicit stack: recursion would overflow on large mazes
  std::vector<std::pair<int, int>> stack;

  const int start_x = rng.uniform_int(0, grid.width() - 1);
  const int start_y = rng.uniform_int(0, grid.height() - 1);
  stack.push_back({start_x, start_y});
  visited[grid.cell_index(start_x, start_y)] = 1;

  while (!stack.empty()) {
    const auto [x, y] = stack.back();
    NeighborList unvisited;
    for (const Neighbor & candidate : grid.neighbors(x, y)) {
      if (!visited[grid.cell_index(candidate.x, candidate.y)]) {
        unvisited.push_back(candidate);
      }
    }
    if (unvisited.empty()) {
      stack.pop_back();  // dead end: backtrack
      continue;
    }
    const auto [nx, ny, dir] = unvisited[rng.index(unvisited.size())];
    grid.carve(x, y, dir);
    visited[grid.cell_index(nx, ny)] = 1;
    stack.push_back({nx, ny});
  }
}

}  // namespace maze_gen
