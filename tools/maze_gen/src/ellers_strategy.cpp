#include <algorithm>
#include <utility>
#include <vector>
#include "strategies.hpp"
#include "union_find.hpp"

namespace maze_gen
{

void EllersStrategy::generate(MazeGrid & grid, Random & rng, const MazeStrategyOptions & options)
{
  const int width = grid.width();
  const int height = grid.height();
  UnionFind sets(grid.cell_count());

  // (set, x) for every cell of the current row; reused for all rows
  std::vector<std::pair<int, int>> row_cells;
  row_cells.reserve(width);

  for (int y = 0; y < height; ++y) {
    const bool last_row = (y == height - 1);

    // 1) Join neighboring cells of different sets: at random, or all of them on the
    //    last row, where nothing below can connect them later
    for (int x = 0; x < width - 1; ++x) {
      const int left = grid.cell_index(x, y);
      const int right = grid.cell_index(x + 1, y);
      if (sets.connected(left, right)) {
        continue;
      }
      if (last_row || rng.chance(options.ellers_join_probability)) {
        grid.carve(x, y, Dir::E);
        sets.unite(left, right);
      }
    }
    if (last_row) {
      break;
    }

    // 2) Every set of the row opens downwards through at least one of its cells.
    //    Sorting puts the cells of each set together, in an order that does not
    //    depend on the standard library (a hash map would)
    row_cells.clear();
    for (int x = 0; x < width; ++x) {
      row_cells.emplace_back(sets.find(grid.cell_index(x, y)), x);
    }
    std::sort(row_cells.begin(), row_cells.end());

    for (size_t first = 0; first < row_cells.size(); ) {
      size_t last = first;
      while (last < row_cells.size() && row_cells[last].first == row_cells[first].first) {
        ++last;
      }
      rng.shuffle(row_cells.begin() + first, row_cells.begin() + last);

      const int down_count = rng.uniform_int(1, static_cast<int>(last - first));
      for (int i = 0; i < down_count; ++i) {
        const int x = row_cells[first + i].second;
        grid.carve(x, y, Dir::S);
        sets.unite(grid.cell_index(x, y), grid.cell_index(x, y + 1));
      }
      first = last;
    }
  }
}

}  // namespace maze_gen
