#pragma once
#include "maze_gen/maze_grid.hpp"
#include "maze_gen/maze_strategy_options.hpp"

namespace maze_gen {

class MazeExitGenerator
{
public:
	MazeExitGenerator();
	// Generate exits based on the maze strategy options
	void generateExits(MazeGrid& maze, const MazeStrategyOptions& opts);
private:
	void generateRandomExits(MazeGrid& maze, const MazeStrategyOptions& opts);
	void generateFixedExits(MazeGrid& maze, const MazeStrategyOptions& opts);
};

}  // namespace maze_gen
