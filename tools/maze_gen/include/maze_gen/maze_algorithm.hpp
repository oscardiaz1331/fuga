#pragma once
#include <optional>
#include <span>
#include <string_view>

namespace maze_gen
{

enum class MazeAlgorithm
{
  DFS,
  AldousBroder,
  CellularAutomaton,
  Ellers,
  BinaryTree,
  GrowingTree,
  HuntAndKill,
  Kruskal,
  Prim,
  RecursiveDivision,
  Sidewinder,
  Wilson,
  Houston
};

// Every algorithm, in the order of the enum
std::span<const MazeAlgorithm> all_algorithms();

// Lower-case name with underscores, such as "hunt_and_kill".
// Throws std::invalid_argument for a value that is not in the enum
std::string_view to_string(MazeAlgorithm algorithm);

// The opposite of to_string. Empty if no algorithm has that exact name
std::optional<MazeAlgorithm> algorithm_from_string(std::string_view name);

}  // namespace maze_gen
