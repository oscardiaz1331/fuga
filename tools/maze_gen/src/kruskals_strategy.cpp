#include <vector>
#include <random>
#include <algorithm>
#include "maze_gen/kruskals_strategy.hpp"
#include "maze_gen/union_find.hpp"

namespace maze_gen {

struct Edge {
    int x, y;
    Dir dir;

    Edge(int x, int y, Dir dir) : x(x), y(y), dir(dir) {}
};

void KruskalsStrategy::generateMaze(MazeGrid& grid) {
    const int width = grid.width();
    const int height = grid.height();
    const int total_cells = width * height;

    UnionFind uf(total_cells);

    std::vector<Edge> edges;

    // Add all vertical and horizontal walls as edges
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (x < width - 1) // horizontal edge to the right
                edges.emplace_back(x, y, Dir::E);
            if (y < height - 1) // vertical edge downward
                edges.emplace_back(x, y, Dir::S);
        }
    }

    auto rng = make_rng();
    std::shuffle(edges.begin(), edges.end(), rng); // Randomized order

    for (const auto& edge : edges) {
        int x = edge.x;
        int y = edge.y;
        int nx = x, ny = y;

        if (edge.dir == Dir::E)
            nx = x + 1;
        else if (edge.dir == Dir::S)
            ny = y + 1;

        if (nx >= width || ny >= height)
            continue;

        int cell1 = y * width + x;
        int cell2 = ny * width + nx;

        if (!uf.connected(cell1, cell2)) {
            uf.unite(cell1, cell2);
            grid.carve(x, y, edge.dir);
        }
    }
}

}  // namespace maze_gen
