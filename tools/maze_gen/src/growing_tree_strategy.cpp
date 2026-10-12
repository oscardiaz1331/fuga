#include "maze_gen/growing_tree_strategy.hpp"
#include <algorithm>
#include <random>

namespace maze_gen {

void GrowingTreeStrategy::generateMaze(MazeGrid& grid)
{
	auto rng = make_rng();
	std::uniform_int_distribution<int> distW(0, grid.width() - 1);
	std::uniform_int_distribution<int> distH(0, grid.height() - 1);
	int x1 = distW(rng);
	int y1 = distH(rng);
	std::vector<std::pair<int, int>> unvisitedCells{ {x1, y1} };
	grid.getVisitedCell(x1, y1).isVisited = true;
	while (!unvisitedCells.empty()) {
		std::shuffle(unvisitedCells.begin(), unvisitedCells.end(), rng);
		auto [x, y] = unvisitedCells.back();
		auto neighbors = grid.neighbors4(x, y);
		std::shuffle(neighbors.begin(), neighbors.end(), rng);
		for (int i = 0; i < neighbors.size(); ++i) {
			auto [nx, ny, dir] = neighbors[i];
			if (!grid.getConstVisitedCell(nx, ny).isVisited) {
				grid.carve(x, y, dir);
				grid.getVisitedCell(nx, ny).isVisited = true;
				unvisitedCells.push_back({ nx, ny });
				break; // Exit the loop after carving one passage
			}
			if (i == neighbors.size() - 1) {
				unvisitedCells.pop_back(); // No unvisited neighbors, remove current cell
			}
		}
	}
}

}  // namespace maze_gen
