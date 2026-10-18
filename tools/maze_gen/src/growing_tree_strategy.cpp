#include <utility>
#include <vector>
#include "strategies.hpp"

namespace maze_gen
{

void GrowingTreeStrategy::generate(
  MazeGrid & grid, Random & rng, const MazeStrategyOptions & options)
{
  std::vector<uint8_t> visited(grid.cell_count(), 0);
  const int start_x = rng.uniform_int(0, grid.width() - 1);
  const int start_y = rng.uniform_int(0, grid.height() - 1);
  // Cells that may still have unvisited neighbors
  std::vector<std::pair<int, int>> active_cells{{start_x, start_y}};
  visited[grid.cell_index(start_x, start_y)] = 1;

  while (!active_cells.empty()) {
    // Which cell grows next is what gives the maze its look. The newest one is already
    // at the back; a random one is swapped there, which avoids shuffling the whole list
    if (!rng.chance(options.growing_tree_newest_ratio)) {
      std::swap(active_cells[rng.index(active_cells.size())], active_cells.back());
    }
    const auto [x, y] = active_cells.back();

    NeighborList unvisited;
    for (const Neighbor & candidate : grid.neighbors(x, y)) {
      if (!visited[grid.cell_index(candidate.x, candidate.y)]) {
        unvisited.push_back(candidate);
      }
    }
    if (unvisited.empty()) {
      active_cells.pop_back();  // nothing left to grow into from this cell
      continue;
    }
    const auto [nx, ny, dir] = unvisited[rng.index(unvisited.size())];
    grid.carve(x, y, dir);
    visited[grid.cell_index(nx, ny)] = 1;
    active_cells.push_back({nx, ny});
  }
}

}  // namespace maze_gen
