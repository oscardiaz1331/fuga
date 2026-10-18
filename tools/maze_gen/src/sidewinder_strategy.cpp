#include <vector>
#include "strategies.hpp"

namespace maze_gen
{

void SidewinderStrategy::generate(
  MazeGrid & grid, Random & rng, const MazeStrategyOptions & options)
{
  const int width = grid.width();
  const int height = grid.height();

  std::vector<int> run;
  run.reserve(width);

  for (int y = 0; y < height; ++y) {
    run.clear();
    for (int x = 0; x < width; ++x) {
      run.push_back(x);

      const bool at_east_border = (x == width - 1);
      bool carve_east;
      if (y == 0) {
        // The top row has nowhere to go north: it is one single corridor
        carve_east = !at_east_border;
      } else {
        carve_east = !at_east_border && rng.chance(options.sidewinder_east_probability);
      }

      if (carve_east) {
        grid.carve(x, y, Dir::E);
        continue;
      }
      // Close the run: one of its cells, chosen at random, opens to the north
      if (y > 0) {
        grid.carve(run[rng.index(run.size())], y, Dir::N);
      }
      run.clear();
    }
  }
}

}  // namespace maze_gen
