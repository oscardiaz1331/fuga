#include <vector>
#include "strategies.hpp"

namespace maze_gen
{

void AldousBroderStrategy::generate(MazeGrid & grid, Random & rng, const MazeStrategyOptions &)
{
  std::vector<uint8_t> visited(grid.cell_count(), 0);
  int x = rng.uniform_int(0, grid.width() - 1);
  int y = rng.uniform_int(0, grid.height() - 1);
  visited[grid.cell_index(x, y)] = 1;
  int cells_visited = 1;

  // A plain random walk: a wall only comes down the first time a cell is entered
  while (cells_visited < grid.cell_count()) {
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
}

}  // namespace maze_gen
