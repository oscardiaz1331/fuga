#include "maze_gen/hunt_and_kill_strategy.hpp"
#include <algorithm>
#include <random>

namespace maze_gen {

void HuntAndKillStrategy::generateMaze(MazeGrid& grid)
{
	auto rng = make_rng();
	std::uniform_int_distribution<int> distW(0, grid.width() - 1);
	std::uniform_int_distribution<int> distH(0, grid.height() - 1);
	int x = distW(rng);
	int y = distH(rng);
	grid.getVisitedCell(x, y).isVisited = true;
	int cellsVisited = 1;
	int width = grid.width();
	int height = grid.height();
	while (cellsVisited < width * height) {
		auto neighbors = grid.neighbors4(x, y);
		std::shuffle(neighbors.begin(), neighbors.end(), rng);
		bool found = false;
		for (const auto& [nx, ny, dir] : neighbors) {
			if (!grid.getConstVisitedCell(nx, ny).isVisited) {
				grid.carve(x, y, dir);
				grid.getVisitedCell(nx, ny).isVisited = true;
				x = nx;
				y = ny;
				cellsVisited++;
				found = true;
				break; // Exit the loop after carving one passage
			}
		}
		if (!found) { // Hunt phase
			for (int x_h = 0; x_h < width; ++x_h) {
				for (int y_h = 0; y_h < height; ++y_h) {
					if (!grid.getConstVisitedCell(x_h, y_h).isVisited) {
						x = x_h;
						y = y_h;
						grid.getVisitedCell(x, y).isVisited = true;
						cellsVisited++;
						found = true;
						break; // Exit the loop after finding an unvisited cell
					}
				}
				if (found) break; // Exit outer loop if found
			}
		}
	}
}

}  // namespace maze_gen
