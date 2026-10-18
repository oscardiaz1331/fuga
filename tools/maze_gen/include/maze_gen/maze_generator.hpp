#pragma once
#include <memory>
#include "maze_gen/maze_algorithm.hpp"
#include "maze_gen/maze_grid.hpp"
#include "maze_gen/maze_strategy_options.hpp"

namespace maze_gen
{

class MazeGenerator
{
public:
  explicit MazeGenerator(MazeAlgorithm algorithm = MazeAlgorithm::DFS);
  ~MazeGenerator();
  MazeGenerator(MazeGenerator &&) noexcept;
  MazeGenerator & operator=(MazeGenerator &&) noexcept;

  void set_algorithm(MazeAlgorithm algorithm);
  MazeAlgorithm algorithm() const;

  // Build a maze in a grid, replacing whatever it had.
  // Throws std::invalid_argument, leaving the grid untouched, if an option is out of range
  void generate(MazeGrid & grid, const MazeStrategyOptions & options);
  // Same, on a new grid of that size
  MazeGrid generate(int width, int height, const MazeStrategyOptions & options);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace maze_gen
