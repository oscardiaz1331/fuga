#include "maze_gen/wilsons_strategy.hpp"
#include <random>
#include <vector>
#include <tuple>
#include <cassert>

namespace maze_gen {

void WilsonsStrategy::generateMaze(MazeGrid& grid)
{
    const int W = grid.width();
    const int H = grid.height();
    if (W <= 0 || H <= 0) return;

    const int total = W * H;
    auto rng = make_rng();
    std::uniform_int_distribution<int> cell_dist(0, total - 1);

    // in_tree[id] == 1 means the cell is already in the UST
    std::vector<uint8_t> in_tree(total, 0);

    // pos_in_path[id] gives the index inside 'path' where this cell appears, or -1 if not in path
    std::vector<int> pos_in_path(total, -1);

    std::vector<Step> path;
    path.reserve(std::max(64, std::min(1024, total / 10))); // heuristic reserve

    // 1) Choose a random cell and add it to the UST
    int seed = cell_dist(rng);
    in_tree[seed] = 1;
    int in_tree_count = 1;

    auto id_from = [&](int x, int y) { return y * W + x; };

    while (in_tree_count < total) {
        // 2) Pick a random cell not yet in the tree
        int start;
        do {
            start = cell_dist(rng);
        } while (in_tree[start]);

        // initialize walk from start
        path.clear();
        std::fill(pos_in_path.begin(), pos_in_path.end(), -1);

        int cur_id = start;
        int curx = cur_id % W;
        int cury = cur_id / W;

        // 3) Perform a random walk until we hit a cell in the tree
        while (!in_tree[cur_id]) {
            // get 4-neighbors
            auto neigh = grid.neighbors4(curx, cury); // vector<tuple<int,int,Dir>>
            assert(!neigh.empty());

            // pick random neighbor uniformly
            std::uniform_int_distribution<int> nd(0, static_cast<int>(neigh.size()) - 1);
            auto [nx, ny, dir] = neigh[nd(rng)];
            int nid = id_from(nx, ny);

            if (pos_in_path[nid] != -1) {
                // Loop detected: erase cycle (keep up to pos_in_path[nid])
                int keep = pos_in_path[nid];
                for (int k = keep + 1; k < (int)path.size(); ++k) {
                    pos_in_path[path[k].id] = -1;
                }
                path.resize(keep + 1);
                // move current to the previously-occurring node (nid)
                cur_id = nid;
                curx = nx;
                cury = ny;
            }
            else {
                // append current cell with direction to chosen neighbor
                Step s;
                s.id = cur_id;
                s.x = curx;
                s.y = cury;
                s.dirToNext = dir;
                pos_in_path[cur_id] = static_cast<int>(path.size());
                path.push_back(s);

                // advance
                cur_id = nid;
                curx = nx;
                cury = ny;
            }
        } // end random walk (cur_id is in_tree)

        // 4) Add the loop-erased path to the tree by carving passages
        for (const auto& step : path) {
            // carve from (step.x, step.y) to direction step.dirToNext
            grid.carve(step.x, step.y, step.dirToNext);

            // mark both cells as in_tree (the current and its neighbor)
            int nx = step.x;
            int ny = step.y;
            grid.move(nx, ny, step.dirToNext);
            int nid = id_from(nx, ny);

            if (!in_tree[step.id]) {
                in_tree[step.id] = 1;
                ++in_tree_count;
            }
            if (!in_tree[nid]) {
                in_tree[nid] = 1;
                ++in_tree_count;
            }
            pos_in_path[step.id] = -1;
        }
        // path cleared effectively by pos_in_path reset; loop continues until all cells are in_tree
    }
}

}  // namespace maze_gen
