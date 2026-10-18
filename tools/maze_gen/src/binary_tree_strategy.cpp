#include "strategies.hpp"

namespace maze_gen
{

void BinaryTreeStrategy::generate(MazeGrid & grid, Random & rng, const MazeStrategyOptions &)
{
  for (int x = 0; x < grid.width(); ++x) {
    for (int y = 0; y < grid.height(); ++y) {
      // Every cell opens north or west. Border cells have a single way to go, and the
      // corner has none
      const bool can_go_north = y > 0;
      const bool can_go_west = x > 0;
      if (can_go_north && can_go_west) {
        grid.carve(x, y, rng.chance(0.5) ? Dir::N : Dir::W);
      } else if (can_go_north) {
        grid.carve(x, y, Dir::N);
      } else if (can_go_west) {
        grid.carve(x, y, Dir::W);
      }
    }
  }
}

}  // namespace maze_gen
