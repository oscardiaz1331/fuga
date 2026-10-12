#include "maze_gen/sidewinder_strategy.hpp"
#include <vector>
#include <random>

namespace maze_gen {

void SidewinderStrategy::generateMaze(MazeGrid& grid)
{
    const int W = grid.width();
    const int H = grid.height();
    if (W <= 0 || H <= 0) return;

    auto rng = make_rng();
    std::uniform_real_distribution<float> prob(0.f, 1.f);

    std::vector<int> run;
    run.reserve(W);

    for (int y = 0; y < H; ++y) {
        run.clear();
        for (int x = 0; x < W; ++x) {
            run.push_back(x);

            const bool at_east = (x == W - 1);
            bool carve_east;

            // Top row: always carve east unless at east boundary (keeps top row connected).
            if (y == 0) {
                carve_east = !at_east;
            } else {
                carve_east = (!at_east) && (prob(rng) < _threshold);
            }

            if (carve_east) {
                grid.carve(x, y, Dir::E);
            } else {
                // Close the run: choose a random cell from run and carve north (if possible)
                if (y > 0 && !run.empty()) {
                    std::uniform_int_distribution<int> idist(0, static_cast<int>(run.size()) - 1);
                    int chosen_x = run[idist(rng)];
                    grid.carve(chosen_x, y, Dir::N);
                }
                run.clear();
            }
        }
    }
}

}  // namespace maze_gen
