#pragma once
#include "maze_gen/i_maze_generation_strategy.hpp"

namespace maze_gen {

class SidewinderStrategy :
    public IMazeGenerationStrategy
{
    public:
		void generateMaze(MazeGrid& grid) override;

private: 
    float _threshold = 0.5f; // Probability threshold for carving east vs north
};

}  // namespace maze_gen
