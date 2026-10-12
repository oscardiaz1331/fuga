#pragma once
#include "maze_gen/i_maze_generation_strategy.hpp"
#include <vector>
#include <unordered_map>

namespace maze_gen {

class EllersStrategy :
    public IMazeGenerationStrategy
{
public:
    void generateMaze(MazeGrid& grid);
private:
    float _threshold = 0.5f;
};

}  // namespace maze_gen
