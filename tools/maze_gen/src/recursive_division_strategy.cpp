#include "maze_gen/recursive_division_strategy.hpp"
#include <random>
#include <algorithm>

namespace maze_gen {

void RecursiveDivisionStrategy::generateMaze(MazeGrid& grid) {
    _rng = make_rng(); // reseed here: the seed is set after construction
    bool initialHorizontal = std::uniform_int_distribution<>(0, 1)(_rng);
    divide(grid, 0, 0, grid.width(), grid.height(), initialHorizontal);
}

void RecursiveDivisionStrategy::divide(MazeGrid& grid, int x, int y, int width, int height, bool horizontal) {
    // Stop if too small to divide further
    if (width < 2 || height < 2) return;

    // Decide orientation based on shape if not forced
    if (width < height) horizontal = true;
    else if (height < width) horizontal = false;
    else horizontal = std::uniform_int_distribution<>(0, 1)(_rng);

    if (horizontal) {
        if (height < 3) return;

        // Pick wall row (must be between y+1 and y+height-2 and odd)
        int wallY = y + 1 + 2 * std::uniform_int_distribution<>(0, (height - 2) / 2)(_rng);

        // Pick passage column (must be even index within region)
        int passageX = x + 2 * std::uniform_int_distribution<>(0, (width - 1) / 2)(_rng);

        // Build horizontal wall
        for (int dx = 0; dx < width; ++dx) {
            int cx = x + dx;
            if (cx != passageX) {
                grid.carve(cx, wallY - 1, Dir::S); // carve south from cell above
            }
        }

        // Recurse above and below
        divide(grid, x, y, width, wallY - y, false);
        divide(grid, x, wallY, width, y + height - wallY, false);
    }
    else { // vertical
        if (width < 3) return;

        // Pick wall column
        int wallX = x + 1 + 2 * std::uniform_int_distribution<>(0, (width - 2) / 2)(_rng);

        // Pick passage row
        int passageY = y + 2 * std::uniform_int_distribution<>(0, (height - 1) / 2)(_rng);

        // Build vertical wall
        for (int dy = 0; dy < height; ++dy) {
            int cy = y + dy;
            if (cy != passageY) {
                grid.carve(wallX - 1, cy, Dir::E); // carve east from cell to left
            }
        }

        // Recurse left and right
        divide(grid, x, y, wallX - x, height, true);
        divide(grid, wallX, y, x + width - wallX, height, true);
    }
}

}  // namespace maze_gen
