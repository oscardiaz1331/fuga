#include <vector>
#include "strategies.hpp"

namespace maze_gen
{

void HuntAndKillStrategy::generate(MazeGrid & grid, Random & rng, const MazeStrategyOptions &)
{
  const int width = grid.width();
  const int height = grid.height();
  std::vector<uint8_t> visited(grid.cell_count(), 0);
  int x = rng.uniform_int(0, width - 1);
  int y = rng.uniform_int(0, height - 1);
  visited[grid.cell_index(x, y)] = 1;
  int cells_visited = 1;
  int hunt_start_x = 0;  // every column before this one is fully visited

  while (cells_visited < grid.cell_count()) {
    // Kill: walk to a random unvisited neighbor for as long as there is one
    NeighborList unvisited;
    for (const Neighbor & candidate : grid.neighbors(x, y)) {
      if (!visited[grid.cell_index(candidate.x, candidate.y)]) {
        unvisited.push_back(candidate);
      }
    }
    if (!unvisited.empty()) {
      const auto [nx, ny, dir] = unvisited[rng.index(unvisited.size())];
      grid.carve(x, y, dir);
      visited[grid.cell_index(nx, ny)] = 1;
      x = nx;
      y = ny;
      ++cells_visited;
      continue;
    }

    // Hunt: scan for an unvisited cell that touches the maze built so far, join it to
    // one of its visited neighbors and carry on walking from there
    bool found = false;
    for (int hunt_x = hunt_start_x; hunt_x < width && !found; ++hunt_x) {
      bool column_done = true;
      for (int hunt_y = 0; hunt_y < height; ++hunt_y) {
        if (visited[grid.cell_index(hunt_x, hunt_y)]) {
          continue;
        }
        column_done = false;

        NeighborList visited_neighbors;
        for (const Neighbor & candidate : grid.neighbors(hunt_x, hunt_y)) {
          if (visited[grid.cell_index(candidate.x, candidate.y)]) {
            visited_neighbors.push_back(candidate);
          }
        }
        if (visited_neighbors.empty()) {
          continue;
        }

        grid.carve(hunt_x, hunt_y, visited_neighbors[rng.index(visited_neighbors.size())].dir);
        x = hunt_x;
        y = hunt_y;
        visited[grid.cell_index(x, y)] = 1;
        ++cells_visited;
        found = true;
        break;
      }
      if (column_done && hunt_x == hunt_start_x) {
        ++hunt_start_x;
      }
    }
    if (!found) {
      break;  // cannot happen on a grid: every unvisited region touches a visited cell
    }
  }
}

}  // namespace maze_gen
