#include <utility>
#include <vector>
#include "strategies.hpp"

namespace maze_gen
{

void PrimsStrategy::generate(MazeGrid & grid, Random & rng, const MazeStrategyOptions &)
{
  std::vector<uint8_t> visited(grid.cell_count(), 0);
  // Walls between a cell of the maze and a cell that may still be outside it:
  // the cell inside and the direction that leads out
  std::vector<Neighbor> frontier;

  auto add_cell = [&](int x, int y) {
      visited[grid.cell_index(x, y)] = 1;
      for (const auto & [nx, ny, dir] : grid.neighbors(x, y)) {
        if (!visited[grid.cell_index(nx, ny)]) {
          frontier.push_back({x, y, dir});
        }
      }
    };

  add_cell(rng.uniform_int(0, grid.width() - 1), rng.uniform_int(0, grid.height() - 1));

  while (!frontier.empty()) {
    // Take a random wall out of the list by swapping it with the last one
    std::swap(frontier[rng.index(frontier.size())], frontier.back());
    const auto [x, y, dir] = frontier.back();
    frontier.pop_back();

    const auto [nx, ny] = MazeGrid::neighbor(x, y, dir);
    if (visited[grid.cell_index(nx, ny)]) {
      continue;  // the other side joined the maze through another wall meanwhile
    }
    grid.carve(x, y, dir);
    add_cell(nx, ny);
  }
}

}  // namespace maze_gen
