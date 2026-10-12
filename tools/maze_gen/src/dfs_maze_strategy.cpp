#include "maze_gen/dfs_maze_strategy.hpp"

namespace maze_gen {

void DFSMazeStrategy::generateMaze(MazeGrid& grid)
{
	auto rng = make_rng();
	std::stack<std::pair<int, int>> stack;
	std::uniform_int_distribution<int> distW(0, grid.width() - 1);
	std::uniform_int_distribution<int> distH(0, grid.height() - 1);
	int startX = distW(rng);
	int startY = distH(rng);
	stack.push({ startX, startY });
	grid.getVisitedCell(startX, startY).isVisited = true;

	while (!stack.empty()) {
		auto [x, y] = stack.top();
		auto neighbors = grid.neighbors4(x, y);
		std::vector<std::tuple<int, int, Dir>> unvisited;
		for (const auto& tup : neighbors) {
			int nx, ny;
			Dir dir;
			std::tie(nx, ny, dir) = tup;
			if (!grid.getConstVisitedCell(nx, ny).isVisited) {
				unvisited.push_back(tup);
			}
		}
		if (!unvisited.empty()) {
			std::uniform_int_distribution<size_t> distN(0, unvisited.size() - 1);
			auto chosen = unvisited[distN(rng)];
			int nx, ny;
			Dir dir;
			std::tie(nx, ny, dir) = chosen;
			grid.carve(x, y, dir);
			grid.getVisitedCell(nx, ny).isVisited = true;
			stack.push({ nx, ny });
		}
		else {
			stack.pop();
		}
	}
}

}  // namespace maze_gen
