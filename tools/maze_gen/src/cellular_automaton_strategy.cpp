#include "maze_gen/cellular_automaton_strategy.hpp"
#include <algorithm>
#include <climits>
#include <cmath> 
#include <cmath> 
#include <queue>

namespace maze_gen {

// In this case, the isAlive flag is used to track if a cell is dead or alive
void CellularAutomatonStrategy::generateMaze(MazeGrid& grid)
{
	_rng = make_rng(); // reseed here: the seed is set after construction
	initializeGrid(grid);
	applyRules(grid);
	ensureConnectivity(grid);
	terminate(grid);
}

void CellularAutomatonStrategy::setDifficulty(int difficulty)
{
	difficulty = std::clamp(difficulty, 0, 10);
	float d = static_cast<float>(difficulty) / 10.0f;
    _initialThreshold = 0.50f + 0.20f * d;  // 50%-70% initial walls
    _iterations = static_cast<int>(std::round(5 + 5 * d));  // 5-55 iterations
   
    if (difficulty <= 5) {
        _num_neighbours = 4;
        _birthThreshold = 0.75;// Birth when 3 neighbors
        _deathThreshold = 0.25;  // Death when 0 neighbors
    }
    else {
		_num_neighbours = 8; // Use 8-connected neighbors for higher difficulty 
        _birthThreshold = 0.4375f + 0.0625f * d;  // Birth when 3.5-4+ neighbors
        _deathThreshold = 0.25f + 0.125f * d;  // Death when <2-3 neighbors
    }

}

void CellularAutomatonStrategy::initializeGrid(MazeGrid& grid)
{
	std::uniform_real_distribution<float> dist{ 0.0f, 1.0f };
	for(int x = 0; x < grid.width(); ++x) {
		for(int y = 0; y < grid.height(); ++y) {
			//if (x == 0 || x == grid.width() - 1 || y == 0 || y == grid.height() - 1) {
			//	grid.at(x, y).isAlive = false; // Border cells are always walls
			//}
			//else {
				float random_value = dist(_rng);
				if (random_value < _initialThreshold) {
					grid.getAliveCell(x, y).isAlive = false; // Cell is "dead"
				}
				else {
					grid.getAliveCell(x, y).isAlive = true; // Cell is "alive"
				}
			//}
		}
	}
}

void CellularAutomatonStrategy::applyRules(MazeGrid& grid)
{
	for(int iter = 0; iter < _iterations; ++iter) {
		MazeGrid aux_grid = grid; // Auxiliary grid to store the next state
		for(int x = 0; x < grid.width(); ++x) {
			for(int y = 0; y < grid.height(); ++y) {
				/*if (x == 0 || x == grid.width() - 1 || y == 0 || y == grid.height() - 1) {
					aux_grid.at(x, y).isAlive = false;
					continue;
				}*/
				int alive_neighbors = 0;
                if(_num_neighbours == 4) { 
                    for (const auto& [nx, ny, dir] : grid.neighbors4(x, y)) {
                        if (grid.getConstAliveCell(nx, ny).isAlive) {
                            ++alive_neighbors;
                        }
                    }
                }
                else {
                    for (const auto& [nx, ny, dir] : grid.neighbors8(x, y)) {
                        if (grid.getConstAliveCell(nx, ny).isAlive) {
                            ++alive_neighbors;
                        }
                    }
                }
				if (grid.getConstAliveCell(x, y).isAlive) {
                    // Cell dies if it has too few neighbors (isolation)
					if (alive_neighbors < static_cast <int>(_num_neighbours * _deathThreshold)) {
						aux_grid.getAliveCell(x, y).isAlive = false; // Cell dies
					}
					else {
						aux_grid.getAliveCell(x, y).isAlive = true; // Cell stays alive
					}
				}
				else
				{
                    // Cell is born if it has enough neighbors but not too many
                    if (alive_neighbors == static_cast<int>(_num_neighbours * _birthThreshold)) {
                        aux_grid.getAliveCell(x, y).isAlive = true; // Born
                    }
                    else {
                        aux_grid.getAliveCell(x, y).isAlive = false; // Stay dead (too isolated)
                    }
				}
			}
		}
		grid = aux_grid; // Update the original grid with the new state
	}
}

void CellularAutomatonStrategy::ensureConnectivity(MazeGrid& grid)
{
    // Find all connected components of alive cells
    std::vector<std::vector<bool>> visited(grid.width(), std::vector<bool>(grid.height(), false));
    std::vector<std::vector<std::pair<int, int>>> components;

    // Find connected components using flood fill
    for (int x = 1; x < grid.width() - 1; ++x) {
        for (int y = 1; y < grid.height() - 1; ++y) {
            if (grid.getConstAliveCell(x, y).isAlive && !visited[x][y]) {
                std::vector<std::pair<int, int>> component;
                std::queue<std::pair<int, int>> queue;

                queue.push({ x, y });
                visited[x][y] = true;

                while (!queue.empty()) {
                    auto [cx, cy] = queue.front();
                    queue.pop();
                    component.push_back({ cx, cy });

                    // Check 8-connected neighbors
                    for (const auto& [nx, ny, dir] : grid.neighbors8(cx, cy)) {
                        if (!visited[nx][ny] && grid.getConstAliveCell(nx, ny).isAlive) {
                            visited[nx][ny] = true;
                            queue.push({ nx, ny });
                        }
                    }
                }

                if (!component.empty()) {
                    components.push_back(component);
                }
            }
        }
    }

    // If no components found, create a minimal path
    if (components.empty()) {
        // Create a simple cross pattern in the center
        int cx = grid.width() / 2;
        int cy = grid.height() / 2;

        for (int x = cx - 1; x <= cx + 1; ++x) {
            if (x > 0 && x < grid.width() - 1) {
                grid.getAliveCell(x, cy).isAlive = true;
            }
        }
        for (int y = cy - 1; y <= cy + 1; ++y) {
            if (y > 0 && y < grid.height() - 1) {
                grid.getAliveCell(cx, y).isAlive = true;
            }
        }
        return;
    }

    // Connect all components to the largest one
    if (components.size() > 1) {
        // Find largest component
        size_t largest_idx = 0;
        for (size_t i = 1; i < components.size(); ++i) {
            if (components[i].size() > components[largest_idx].size()) {
                largest_idx = i;
            }
        }

        // Connect each smaller component to the largest one
        for (size_t i = 0; i < components.size(); ++i) {
            if (i == largest_idx) continue;

            // Find closest points between components
            auto& main_component = components[largest_idx];
            auto& small_component = components[i];

            int min_dist = INT_MAX;
            int best_mx = -1, best_my = -1, best_sx = -1, best_sy = -1;

            for (auto [mx, my] : main_component) {
                for (auto [sx, sy] : small_component) {
                    int dist = std::abs(mx - sx) + std::abs(my - sy);
                    if (dist < min_dist) {
                        min_dist = dist;
                        best_mx = mx; best_my = my;
                        best_sx = sx; best_sy = sy;
                    }
                }
            }

            // Create a path between the closest points
            connectPoints(grid, best_mx, best_my, best_sx, best_sy);
        }
    }
}

void CellularAutomatonStrategy::connectPoints(MazeGrid& grid, int x1, int y1, int x2, int y2)
{
    // Simple L-shaped connection
    int cx = x1, cy = y1;

    // Move horizontally first
    while (cx != x2) {
        grid.getAliveCell(cx, cy).isAlive = true;
        cx += (x2 > cx) ? 1 : -1;
    }

    // Then move vertically
    while (cy != y2) {
        grid.getAliveCell(cx, cy).isAlive = true;
        cy += (y2 > cy) ? 1 : -1;
    }

    // Ensure destination is alive
    grid.getAliveCell(x2, y2).isAlive = true;
}

void CellularAutomatonStrategy::terminate(MazeGrid& grid)
{
	for (int x = 0; x < grid.width(); ++x) {
		for (int y = 0; y < grid.height(); ++y) {
			if (grid.getConstAliveCell(x, y).isAlive) {
				for (const auto& [nx, ny, dir] : grid.neighbors8(x, y)) {
					if (grid.getConstAliveCell(nx, ny).isAlive) {
						grid.carve(x, y, dir); // Carve passage if cell is alive
					}
				}
			}
		}
	}
}

}  // namespace maze_gen
