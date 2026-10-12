#pragma once
#include "maze_gen/i_maze_generation_strategy.hpp"
#include <random>

namespace maze_gen {

class RecursiveDivisionStrategy : public IMazeGenerationStrategy {
public:
    void generateMaze(MazeGrid& grid) override;

private:
    std::mt19937 _rng{ make_rng() };
    void divide(MazeGrid& grid, int x, int y, int width, int height, bool horizontal);
};

}  // namespace maze_gen
