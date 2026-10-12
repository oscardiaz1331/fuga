#include "maze_gen/maze_exit_generator.hpp"
#include <algorithm>
#include <random>
#include <iostream>
#include "maze_gen/random_generator.hpp"
#include <set>

namespace maze_gen {

MazeExitGenerator::MazeExitGenerator()
{
}

void MazeExitGenerator::generateExits(MazeGrid& grid, const MazeStrategyOptions& opts)
{
    if (opts.num_random_exits > 0) {
        generateRandomExits(grid, opts);
    }

    if (!opts.fixed_exits.empty()) {
        generateFixedExits(grid, opts);
    }
}

void MazeExitGenerator::generateRandomExits(MazeGrid& grid, const MazeStrategyOptions& opts)
{
    const int width = grid.width();
    const int height = grid.height();
    if (width <= 0 || height <= 0 || opts.num_random_exits <= 0) return;

    const bool excludeCorners = true;

    std::vector<std::tuple<int, int, Dir>> perimeter;
    // top (y=0)
    for (int x = 0; x < width; ++x) {
        if (excludeCorners && (x == 0 || x == width - 1)) continue;
        perimeter.emplace_back(x, 0, Dir::N);
    }
    // right (x=width-1), y=1..height-2
    for (int y = 1; y <= height - 2; ++y)
        perimeter.emplace_back(width - 1, y, Dir::E);

    // bottom (y=height-1), x=width-1..0
    for (int x = width - 1; x >= 0; --x) {
        if (excludeCorners && (x == 0 || x == width - 1)) continue;
        perimeter.emplace_back(x, height - 1, Dir::S);
    }
    // left (x=0), y=height-2..1
    for (int y = height - 2; y >= 1; --y)
        perimeter.emplace_back(0, y, Dir::W);

    if (perimeter.empty()) return;

    auto rng = RandomGenerator::generateRng(opts.seed);
    std::shuffle(perimeter.begin(), perimeter.end(), rng);

    int take = std::min<int>(opts.num_random_exits, (int)perimeter.size());
    for (int i = 0; i < take; ++i) {
        auto [x, y, d] = perimeter[i];
        grid.carve(x, y, d);
        grid.addExit(x, y, d);
    }
}

void MazeExitGenerator::generateFixedExits(MazeGrid& grid, const MazeStrategyOptions& opts)
{
    const int width = grid.width();
    const int height = grid.height();
    for (auto [x, y, dir] : opts.fixed_exits) {
        // Validate coordinates are on the perimeter
        bool isOnPerimeter = (x == 0 || x == width - 1) || (y == 0 || y == height - 1);
        if (!isOnPerimeter) {
            // ignore invalid fixed exit
            continue;
        }
        // ensure the chosen direction points outside (neighbor is out-of-bounds)
        auto [nx, ny] = grid.neighbor(x, y, dir);
        if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
            // supplied direction doesn't point outside; ignore or optionally flip
            continue;
        }
        grid.carve(x, y, dir);
        grid.addExit(x, y, dir);
    }
}

}  // namespace maze_gen
