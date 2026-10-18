#include <vector>
#include "strategies.hpp"
#include "union_find.hpp"

namespace maze_gen
{

namespace
{

// The wall between a cell and its neighbor to the east or to the south
struct Edge
{
  int x;
  int y;
  Dir dir;
};

}  // namespace

void KruskalsStrategy::generate(MazeGrid & grid, Random & rng, const MazeStrategyOptions &)
{
  const int width = grid.width();
  const int height = grid.height();

  // Every inner wall, once
  std::vector<Edge> edges;
  edges.reserve(static_cast<size_t>(2) * grid.cell_count());
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if (x < width - 1) {
        edges.push_back({x, y, Dir::E});
      }
      if (y < height - 1) {
        edges.push_back({x, y, Dir::S});
      }
    }
  }
  rng.shuffle(edges.begin(), edges.end());

  // Take the walls in random order, opening the ones that join two separate sets
  UnionFind sets(grid.cell_count());
  for (const Edge & edge : edges) {
    const auto [nx, ny] = MazeGrid::neighbor(edge.x, edge.y, edge.dir);
    const int cell = grid.cell_index(edge.x, edge.y);
    const int other = grid.cell_index(nx, ny);
    if (!sets.connected(cell, other)) {
      sets.unite(cell, other);
      grid.carve(edge.x, edge.y, edge.dir);
    }
  }
}

}  // namespace maze_gen
