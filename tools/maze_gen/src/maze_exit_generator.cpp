#include "maze_exit_generator.hpp"
#include <algorithm>

namespace maze_gen
{

std::vector<Exit> MazeExitGenerator::random_exit_slots(const MazeGrid & grid)
{
  const int width = grid.width();
  const int height = grid.height();
  std::vector<Exit> slots;

  // Clockwise from the top-left, skipping the four corner cells
  for (int x = 1; x <= width - 2; ++x) {
    slots.push_back({x, 0, Dir::N});
  }
  for (int y = 1; y <= height - 2; ++y) {
    slots.push_back({width - 1, y, Dir::E});
  }
  for (int x = width - 2; x >= 1; --x) {
    slots.push_back({x, height - 1, Dir::S});
  }
  for (int y = height - 2; y >= 1; --y) {
    slots.push_back({0, y, Dir::W});
  }
  return slots;
}

void MazeExitGenerator::generate_exits(
  MazeGrid & grid, Random & rng, const MazeStrategyOptions & options) const
{
  for (const Exit & exit : options.fixed_exits) {
    grid.add_exit(exit.x, exit.y, exit.dir);
  }

  if (options.num_random_exits <= 0) {
    return;
  }
  // A fixed exit may already sit on one of the slots
  std::vector<Exit> slots = random_exit_slots(grid);
  const auto & taken = grid.exits();
  std::erase_if(
    slots, [&taken](const Exit & slot) {
      return std::find(taken.begin(), taken.end(), slot) != taken.end();
    });

  rng.shuffle(slots.begin(), slots.end());
  const size_t count = std::min(slots.size(), static_cast<size_t>(options.num_random_exits));
  for (size_t i = 0; i < count; ++i) {
    grid.add_exit(slots[i].x, slots[i].y, slots[i].dir);
  }
}

}  // namespace maze_gen
