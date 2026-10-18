#include "region_connector.hpp"
#include <algorithm>
#include <tuple>
#include <vector>
#include "union_find.hpp"

namespace maze_gen
{

namespace
{

// Region id of every cell: cells joined by open passages share an id
std::vector<int> label_regions(const MazeGrid & grid, int & region_count)
{
  const int total = grid.cell_count();
  std::vector<int> labels(total, -1);
  std::vector<int> queue;
  queue.reserve(total);
  region_count = 0;

  for (int start = 0; start < total; ++start) {
    if (labels[start] != -1) {
      continue;
    }
    queue.clear();
    queue.push_back(start);
    labels[start] = region_count;
    for (size_t head = 0; head < queue.size(); ++head) {
      const int x = queue[head] % grid.width();
      const int y = queue[head] / grid.width();
      for (const auto & [nx, ny, dir] : grid.neighbors(x, y)) {
        const int neighbor = grid.cell_index(nx, ny);
        if (labels[neighbor] == -1 && !grid.has_wall(x, y, dir)) {
          labels[neighbor] = region_count;
          queue.push_back(neighbor);
        }
      }
    }
    ++region_count;
  }
  return labels;
}

// A pair of neighboring cells that ended up assigned to two different regions
struct Bridge
{
  int walls;  // walls to break to join both regions through this pair
  int cell;
  Dir dir;    // from cell to the cell on the other side

  bool operator<(const Bridge & other) const
  {
    return std::tie(walls, cell, dir) < std::tie(other.walls, other.cell, other.dir);
  }
};

}  // namespace

int RegionConnector::connect(MazeGrid & grid) const
{
  const int width = grid.width();
  const int total = grid.cell_count();

  int region_count = 0;
  const std::vector<int> labels = label_regions(grid, region_count);
  if (region_count <= 1) {
    return 0;
  }

  // Regions that have to end up connected: the ones with two or more cells, and the
  // ones holding an exit even if it is a single sealed cell
  std::vector<int> region_size(region_count, 0);
  for (int label : labels) {
    ++region_size[label];
  }
  std::vector<char> wanted(region_count, 0);
  for (int region = 0; region < region_count; ++region) {
    wanted[region] = region_size[region] > 1 ? 1 : 0;
  }
  for (const Exit & exit : grid.exits()) {
    wanted[labels[grid.cell_index(exit.x, exit.y)]] = 1;
  }
  if (std::count(wanted.begin(), wanted.end(), 1) <= 1) {
    return 0;
  }

  // Grow all the wanted regions at once, one cell at a time. Everything outside them is
  // solid rock, so every step breaks exactly one wall: a plain breadth-first search
  // leaves each cell owned by the region that reaches it breaking the fewest walls
  std::vector<int> cost(total, 0);
  std::vector<int> owner(total, -1);
  std::vector<int> parent(total, -1);
  std::vector<int> queue;
  queue.reserve(total);
  for (int cell = 0; cell < total; ++cell) {
    if (wanted[labels[cell]]) {
      owner[cell] = labels[cell];
      queue.push_back(cell);
    }
  }
  for (size_t head = 0; head < queue.size(); ++head) {
    const int current = queue[head];
    for (const auto & [nx, ny, dir] : grid.neighbors(current % width, current / width)) {
      const int neighbor = grid.cell_index(nx, ny);
      if (owner[neighbor] != -1) {
        continue;
      }
      cost[neighbor] = cost[current] + 1;
      owner[neighbor] = owner[current];
      parent[neighbor] = current;
      queue.push_back(neighbor);
    }
  }

  // Where two territories meet there is a possible bridge between their regions.
  // Looking east and south only, so each pair of cells is seen once
  std::vector<Bridge> bridges;
  for (int cell = 0; cell < total; ++cell) {
    const int x = cell % width;
    const int y = cell / width;
    for (Dir dir : {Dir::E, Dir::S}) {
      const auto [nx, ny] = MazeGrid::neighbor(x, y, dir);
      if (!grid.in_bounds(nx, ny)) {
        continue;
      }
      const int other = grid.cell_index(nx, ny);
      if (owner[cell] != owner[other]) {
        const int wall = grid.has_wall(x, y, dir) ? 1 : 0;
        bridges.push_back({cost[cell] + cost[other] + wall, cell, dir});
      }
    }
  }
  std::sort(bridges.begin(), bridges.end());

  // Carve from a cell back to the region that owns it
  int walls_broken = 0;
  auto carve_back = [&](int cell) {
      while (parent[cell] != -1) {
        const int previous = parent[cell];
        const int px = previous % width;
        const int py = previous / width;
        for (const auto & [nx, ny, dir] : grid.neighbors(px, py)) {
          if (grid.cell_index(nx, ny) == cell && grid.has_wall(px, py, dir)) {
            grid.carve(px, py, dir);
            ++walls_broken;
          }
        }
        cell = previous;
      }
    };

  // Cheapest bridges first, skipping the ones between regions that are already joined:
  // a minimum spanning tree over the regions
  UnionFind joined(region_count);
  for (const Bridge & bridge : bridges) {
    const int x = bridge.cell % width;
    const int y = bridge.cell / width;
    const auto [nx, ny] = MazeGrid::neighbor(x, y, bridge.dir);
    const int other = grid.cell_index(nx, ny);
    if (joined.connected(owner[bridge.cell], owner[other])) {
      continue;
    }
    joined.unite(owner[bridge.cell], owner[other]);
    carve_back(bridge.cell);
    carve_back(other);
    if (grid.has_wall(x, y, bridge.dir)) {
      grid.carve(x, y, bridge.dir);
      ++walls_broken;
    }
  }
  return walls_broken;
}

}  // namespace maze_gen
