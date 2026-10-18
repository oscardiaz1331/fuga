#pragma once
#include "maze_gen/maze_grid.hpp"
#include "maze_gen/maze_strategy_options.hpp"
#include "random.hpp"

namespace maze_gen
{

// One generation algorithm. It has no state of its own: everything it needs comes in
class MazeStrategy
{
public:
  virtual ~MazeStrategy() = default;

  // Build the maze in a grid that has every wall in place
  virtual void generate(MazeGrid & grid, Random & rng, const MazeStrategyOptions & options) = 0;
};

}  // namespace maze_gen
