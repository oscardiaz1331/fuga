#pragma once
#include <vector>
#include "maze_gen/maze_grid.hpp"
#include "maze_gen/maze_strategy_options.hpp"
#include "random.hpp"

namespace maze_gen
{

// Opens the exits of a maze on its outer wall
class MazeExitGenerator
{
public:
  // Every place where a random exit may go: each outer side of the border cells that
  // are not corners
  static std::vector<Exit> random_exit_slots(const MazeGrid & grid);

  // Open the fixed exits first and then the random ones, in the slots still free.
  // The options have to be valid already: see MazeGenerator::generate
  void generate_exits(MazeGrid & grid, Random & rng, const MazeStrategyOptions & options) const;
};

}  // namespace maze_gen
