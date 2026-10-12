#pragma once
#include <random>
#include <stack>
#include "maze_gen/i_maze_generation_strategy.hpp"

namespace maze_gen {

class DFSMazeStrategy :
    public IMazeGenerationStrategy
{
    void generateMaze(MazeGrid& grid) override;
};

}  // namespace maze_gen
