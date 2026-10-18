#include <vector>
#include "strategies.hpp"

namespace maze_gen
{

namespace
{

struct Step
{
  int cell;
  int x;
  int y;
  Dir dir_to_next;  // direction from this cell to the next one in the walk
};

}  // namespace

void wilson_complete(
  MazeGrid & grid, Random & rng, std::vector<uint8_t> & in_tree, int in_tree_count)
{
  const int total = grid.cell_count();
  // Position of every cell inside the current walk, or -1 if it is not part of it
  std::vector<int> pos_in_path(total, -1);
  std::vector<Step> path;
  int next_start = 0;

  while (in_tree_count < total) {
    // The walks may start anywhere without changing which mazes can come out, so the
    // free cells are simply taken in order: probing at random gets slow near the end
    while (in_tree[next_start]) {
      ++next_start;
    }
    int cell = next_start;
    int x = cell % grid.width();
    int y = cell / grid.width();
    path.clear();

    // Random walk until it touches the maze
    while (!in_tree[cell]) {
      const auto neighbors = grid.neighbors(x, y);
      const auto [nx, ny, dir] = neighbors[rng.index(neighbors.size())];
      const int next_cell = grid.cell_index(nx, ny);

      if (pos_in_path[next_cell] != -1) {
        // The walk closed a loop: erase it, including the old step out of next_cell,
        // which is added again with its new direction on the next iteration
        const int keep = pos_in_path[next_cell];
        for (int k = keep; k < static_cast<int>(path.size()); ++k) {
          pos_in_path[path[k].cell] = -1;
        }
        path.resize(keep);
      } else {
        pos_in_path[cell] = static_cast<int>(path.size());
        path.push_back({cell, x, y, dir});
      }
      cell = next_cell;
      x = nx;
      y = ny;
    }

    // Carve the walk and add its cells to the maze
    for (const Step & step : path) {
      grid.carve(step.x, step.y, step.dir_to_next);
      in_tree[step.cell] = 1;
      ++in_tree_count;
      pos_in_path[step.cell] = -1;
    }
  }
}

void WilsonsStrategy::generate(MazeGrid & grid, Random & rng, const MazeStrategyOptions &)
{
  std::vector<uint8_t> in_tree(grid.cell_count(), 0);
  in_tree[rng.index(in_tree.size())] = 1;
  wilson_complete(grid, rng, in_tree, 1);
}

}  // namespace maze_gen
