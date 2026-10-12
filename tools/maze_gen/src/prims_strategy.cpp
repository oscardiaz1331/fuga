#include "maze_gen/prims_strategy.hpp"
#include <random>
#include <algorithm>

namespace maze_gen {

void PrimsStrategy::generateMaze(MazeGrid& grid) {
    const int width = grid.width();
    const int height = grid.height();

    auto rng = make_rng();

    // Start at random cell
    int startX = rng() % width;
    int startY = rng() % height;
    _visited.clear();
    _edges.clear();

    auto index = toIndex(startX, startY, width);
    _visited.insert(index);

    // Add all neighbors of the starting cell
    for (auto dir : grid.directions4()) {
        int nx = startX, ny = startY;
        grid.move(nx, ny, dir); // Helper to move in a direction
        if (isInside(nx, ny, width, height)) {
            _edges.emplace_back(startX, startY, dir);
        }
    }

    while (!_edges.empty()) {
        // Pick random edge
        std::uniform_int_distribution<int> dist(0, _edges.size() - 1);
        int i = dist(rng);
        auto [x, y, dir] = _edges[i];

        std::swap(_edges[i], _edges.back());
        _edges.pop_back();

        int nx = x, ny = y;
        grid.move(nx, ny, dir);

        int neighborIndex = toIndex(nx, ny, width);
        if (_visited.contains(neighborIndex)) continue;

        grid.carve(x, y, dir); // Open wall
        _visited.insert(neighborIndex);

        // Add all walls of the new cell
        for (auto new_dir : grid.directions4()) {
            int nnx = nx, nny = ny;
            grid.move(nnx, nny, new_dir);
            if (isInside(nnx, nny, width, height)) {
                if (!_visited.contains(toIndex(nnx, nny, width))) {
                    _edges.emplace_back(nx, ny, new_dir);
                }
            }
        }
    }
}

bool PrimsStrategy::isInside(int x, int y, int width, int height) {
    return x >= 0 && y >= 0 && x < width && y < height;
}

int PrimsStrategy::toIndex(int x, int y, int width) {
    return y * width + x;
}

}  // namespace maze_gen
