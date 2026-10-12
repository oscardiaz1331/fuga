#pragma once
#include "maze_gen/i_maze_generation_strategy.hpp"

namespace maze_gen {

class WilsonsStrategy :
    public IMazeGenerationStrategy
{
public:
    void generateMaze(MazeGrid& grid) override;

private:
    struct Step {
        int id;
        int x, y;
        Dir dirToNext; // direction from this cell to the next cell in the walk
    };
};

}  // namespace maze_gen
