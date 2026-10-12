#include "maze_gen/binary_tree_strategy.hpp"

namespace maze_gen {

void BinaryTreeStrategy::generateMaze(MazeGrid& grid)
{
	for(int i = 0; i < grid.width(); ++i) {
		for(int j = 0; j < grid.height(); ++j) {
			Dir dir = choose_random_option();
			grid.carve(i, j, dir);
		}
	}
}

Dir BinaryTreeStrategy::choose_random_option()
{
	if (_dist(_rng) == 0) {
		return Dir::N;
	}
	else {
		return Dir::W;
	}
}

}  // namespace maze_gen
