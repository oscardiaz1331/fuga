#include "maze_gen/ellers_strategy.hpp"
#include <algorithm>
#include "maze_gen/union_find.hpp"
#include <random>

namespace maze_gen {

void EllersStrategy::generateMaze(MazeGrid& grid)
{
    const int width = grid.width();
    const int height = grid.height();
    const int total_cells = width * height;
    auto rng = make_rng();
    std::uniform_real_distribution<float> prob(0.f, 1.f);

    UnionFind uf(total_cells);

    auto cellId = [width](int y, int x) { return y * width + x; };

    // Process each row
    for (int y = 0; y < height; ++y) {
        // 1) Join adjacent cells horizontally (randomly) within the row
        for (int x = 0; x < width - 1; ++x) {
            int idLeft = cellId(y, x);
            int idRight = cellId(y, x + 1);

            if (!uf.connected(idLeft, idRight) && prob(rng) < _threshold) {
                grid.carve(x, y, Dir::E);
                uf.unite(idLeft, idRight);
            }
        }

        // 2) If not last row, carve vertical connections downwards, at least one per set
        if (y < height - 1) {
            std::unordered_map<int, std::vector<int>> sets; // root_id -> list of x in this row

            for (int x = 0; x < width; ++x) {
                int id = cellId(y, x);
                int root = uf.find(id);
                sets[root].push_back(x);
            }

            // For each set, choose at least one cell to carve down, maybe more
            for (auto& [root, cells] : sets) {
                std::shuffle(cells.begin(), cells.end(), rng);

                // number of down-connections: uniform between 1 and cells.size()
                std::uniform_int_distribution<int> downDist(1, static_cast<int>(cells.size()));
                int num_down = downDist(rng);

                for (int i = 0; i < num_down; ++i) {
                    int x = cells[i];
                    // carve south from (x, y) to (x, y+1)
                    grid.carve(x, y, Dir::S);
                    int idAbove = cellId(y, x);
                    int idBelow = cellId(y + 1, x);
                    uf.unite(idAbove, idBelow);
                }
            }
            // After this, next row cells exist as their own ids (if not united they remain separate)
        }
    }

    // 3) Last row: ensure all adjacent cells horizontally are connected
    int y = height - 1;
    for (int x = 0; x < width - 1; ++x) {
        int idLeft = cellId(y, x);
        int idRight = cellId(y, x + 1);
        if (!uf.connected(idLeft, idRight)) {
            grid.carve(x, y, Dir::E);
            uf.unite(idLeft, idRight);
        }
    }
}

}  // namespace maze_gen
