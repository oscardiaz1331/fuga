#pragma once
#include "maze_gen/i_maze_generation_strategy.hpp"

namespace maze_gen {

class HuntAndKillStrategy :
    public IMazeGenerationStrategy
{
    public:
		void generateMaze(MazeGrid& grid) override;
};

}  // namespace maze_gen
