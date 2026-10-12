#pragma once
#include <cstdint>
#include <optional>
#include <tuple>
#include <vector>
#include "maze_gen/cell.hpp"

namespace maze_gen {

struct MazeStrategyOptions {
    int difficulty = 0;
    std::optional<uint64_t> seed; // if present, seed RNG
	int num_random_exits = 1; // Number of exits to create in the maze
	std::vector<std::tuple<int, int, Dir>> fixed_exits	; // Coordinates for fixed exits
};

}  // namespace maze_gen
