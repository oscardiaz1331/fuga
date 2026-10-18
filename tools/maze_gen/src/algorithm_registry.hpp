#pragma once
#include <memory>
#include <string_view>
#include "maze_gen/maze_algorithm.hpp"
#include "maze_strategy.hpp"

namespace maze_gen
{

// Everything the library knows about one algorithm. Adding an algorithm means a value
// in MazeAlgorithm, a strategy class and a row in the table of algorithm_registry.cpp
struct AlgorithmInfo
{
  MazeAlgorithm id;
  std::string_view name;
  std::unique_ptr<MazeStrategy>(* create)();
  // Builds a perfect maze: one region and no loops. Only the cave generator does not,
  // and it is the only one whose result may need its regions joined afterwards
  bool perfect;
};

// Throws std::invalid_argument for a value that is not in the enum
const AlgorithmInfo & algorithm_info(MazeAlgorithm algorithm);

}  // namespace maze_gen
