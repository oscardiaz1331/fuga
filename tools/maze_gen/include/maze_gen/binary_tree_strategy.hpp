#pragma once
#include "maze_gen/i_maze_generation_strategy.hpp"
#include <random>

namespace maze_gen {

class BinaryTreeStrategy :
    public IMazeGenerationStrategy
{
public:
    void generateMaze(MazeGrid& grid) override;
private:
    Dir choose_random_option();
    std::mt19937 _rng{make_rng()};
    std::uniform_int_distribution<size_t> _dist{0, 1};
};

}  // namespace maze_gen
