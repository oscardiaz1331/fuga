#include "maze_gen/aldous_broder_strategy.hpp"
#include <random>

namespace maze_gen {

void AldousBroderStrategy::generateMaze(MazeGrid& grid)
{
	auto rng = make_rng();
	std::uniform_int_distribution<int> distW(0, grid.width() - 1);
	std::uniform_int_distribution<int> distH(0, grid.height() - 1);
	int x = distW(rng);
	int y = distH(rng);
	int numTotalCells = grid.width() * grid.height();
	grid.getVisitedCell(x, y).isVisited = true;
	int cellsVisited = 1;
	while (cellsVisited < numTotalCells) {
		auto neighbors = grid.neighbors4(x, y);
		std::uniform_int_distribution<size_t> distN(0, neighbors.size() - 1);
		auto [nx, ny, dir] = neighbors[distN(rng)];
		if (!grid.getConstVisitedCell(nx, ny).isVisited)
		{
			grid.carve(x, y, dir);
			grid.getVisitedCell(nx, ny).isVisited = true;
			++cellsVisited;
		}
		x = nx;
		y = ny;
	}
}

}  // namespace maze_gen
