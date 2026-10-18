#include <algorithm>
#include <vector>
#include "strategies.hpp"

namespace maze_gen
{

// Aldous-Broder is quick while most cells are unvisited and crawls at the end; Wilson
// is the other way round. Houston starts with one and finishes with the other.
void HoustonStrategy::generate(MazeGrid & grid, Random & rng, const MazeStrategyOptions &)
{
  const int total = grid.cell_count();
  std::vector<uint8_t> visited(total, 0);
  int x = rng.uniform_int(0, grid.width() - 1);
  int y = rng.uniform_int(0, grid.height() - 1);
  visited[grid.cell_index(x, y)] = 1;
  int cells_visited = 1;

  // Aldous-Broder until a third of the cells are part of the maze
  const int switch_at = std::max(1, total / 3);
  while (cells_visited < switch_at) {
    const auto neighbors = grid.neighbors(x, y);
    const auto [nx, ny, dir] = neighbors[rng.index(neighbors.size())];
    if (!visited[grid.cell_index(nx, ny)]) {
      grid.carve(x, y, dir);
      visited[grid.cell_index(nx, ny)] = 1;
      ++cells_visited;
    }
    x = nx;
    y = ny;
  }

  wilson_complete(grid, rng, visited, cells_visited);
}

}  // namespace maze_gen
