#include <algorithm>
#include <vector>
#include "strategies.hpp"

namespace maze_gen
{

namespace
{

// One flag per cell, in row-major order: 1 = alive (open), 0 = dead (solid rock)
using Cells = std::vector<uint8_t>;

constexpr int kDx[4] = {0, 1, 0, -1};
constexpr int kDy[4] = {-1, 0, 1, 0};

// A dead cell comes alive with exactly this many alive neighbors, and an alive cell
// dies with fewer than kSurviveMin
constexpr int kBirthCount = 3;
constexpr int kSurviveMin = 1;
// The rule settles after a few rounds: more of them change almost nothing
constexpr int kIterations = 5;

// The level only sets how much rock there is to start with, from 35% to 70% of the
// cells. The rule is the same for every level, so the rock left in the result grows
// steadily with it: about 16% at level 0 and 52% at level 10
double initial_dead_probability(int cave_level)
{
  return 0.35 + 0.035 * cave_level;
}

void initialize(Cells & alive, Random & rng, double dead_probability)
{
  for (uint8_t & cell : alive) {
    cell = rng.chance(dead_probability) ? 0 : 1;
  }
}

void apply_rules(Cells & alive, int width, int height, int iterations)
{
  // Two buffers swapped after every iteration, instead of copying the grid
  Cells next(alive.size(), 0);
  for (int iteration = 0; iteration < iterations; ++iteration) {
    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) {
        // Cells outside the grid count as dead
        int alive_neighbors = 0;
        for (int k = 0; k < 4; ++k) {
          const int nx = x + kDx[k];
          const int ny = y + kDy[k];
          if (nx >= 0 && ny >= 0 && nx < width && ny < height && alive[ny * width + nx]) {
            ++alive_neighbors;
          }
        }
        const int index = y * width + x;
        if (alive[index]) {
          next[index] = alive_neighbors >= kSurviveMin ? 1 : 0;
        } else {
          next[index] = alive_neighbors == kBirthCount ? 1 : 0;
        }
      }
    }
    alive.swap(next);
  }
}

void connect_points(Cells & alive, int width, int x1, int y1, int x2, int y2)
{
  // L-shaped corridor: along the row first, then along the column
  int x = x1;
  int y = y1;
  while (x != x2) {
    alive[y * width + x] = 1;
    x += (x2 > x) ? 1 : -1;
  }
  while (y != y2) {
    alive[y * width + x] = 1;
    y += (y2 > y) ? 1 : -1;
  }
  alive[y2 * width + x2] = 1;
}

// Join every group of alive cells to the largest one
void ensure_connectivity(Cells & alive, int width, int height)
{
  const int total = width * height;

  // Label the groups with a flood fill. Two cells belong together only if they share a
  // side: touching by a corner is not a way through
  std::vector<int> component(total, -1);
  std::vector<int> component_size;
  std::vector<int> queue;
  queue.reserve(total);
  for (int start = 0; start < total; ++start) {
    if (!alive[start] || component[start] != -1) {
      continue;
    }
    const int id = static_cast<int>(component_size.size());
    queue.clear();
    queue.push_back(start);
    component[start] = id;
    for (size_t head = 0; head < queue.size(); ++head) {
      const int cx = queue[head] % width;
      const int cy = queue[head] / width;
      for (int k = 0; k < 4; ++k) {
        const int nx = cx + kDx[k];
        const int ny = cy + kDy[k];
        if (nx < 0 || ny < 0 || nx >= width || ny >= height) {
          continue;
        }
        const int neighbor = ny * width + nx;
        if (alive[neighbor] && component[neighbor] == -1) {
          component[neighbor] = id;
          queue.push_back(neighbor);
        }
      }
    }
    component_size.push_back(static_cast<int>(queue.size()));
  }

  if (component_size.size() <= 1) {
    return;
  }

  const int largest = static_cast<int>(
    std::max_element(component_size.begin(), component_size.end()) - component_size.begin());

  // One breadth-first search from the whole largest group gives, for every cell, its
  // Manhattan distance to that group and which of its cells is the closest
  std::vector<int> distance(total, -1);
  std::vector<int> origin(total, -1);
  queue.clear();
  for (int i = 0; i < total; ++i) {
    if (component[i] == largest) {
      distance[i] = 0;
      origin[i] = i;
      queue.push_back(i);
    }
  }
  for (size_t head = 0; head < queue.size(); ++head) {
    const int current = queue[head];
    const int cx = current % width;
    const int cy = current / width;
    for (int k = 0; k < 4; ++k) {
      const int nx = cx + kDx[k];
      const int ny = cy + kDy[k];
      if (nx < 0 || ny < 0 || nx >= width || ny >= height) {
        continue;
      }
      const int neighbor = ny * width + nx;
      if (distance[neighbor] != -1) {
        continue;
      }
      distance[neighbor] = distance[current] + 1;
      origin[neighbor] = origin[current];
      queue.push_back(neighbor);
    }
  }

  // The closest cell of every smaller group gets a corridor to the largest one
  std::vector<int> closest(component_size.size(), -1);
  for (int i = 0; i < total; ++i) {
    const int id = component[i];
    if (id == -1 || id == largest) {
      continue;
    }
    if (closest[id] == -1 || distance[i] < distance[closest[id]]) {
      closest[id] = i;
    }
  }
  for (int target : closest) {
    if (target == -1) {
      continue;
    }
    const int from = origin[target];
    connect_points(alive, width, from % width, from / width, target % width, target / width);
  }
}

// Turn the alive cells into open space: no wall between two alive neighbors
void carve_cave(MazeGrid & grid, const Cells & alive)
{
  for (int y = 0; y < grid.height(); ++y) {
    for (int x = 0; x < grid.width(); ++x) {
      if (!alive[grid.cell_index(x, y)]) {
        continue;
      }
      for (const auto & [nx, ny, dir] : grid.neighbors(x, y)) {
        if (alive[grid.cell_index(nx, ny)]) {
          grid.carve(x, y, dir);
        }
      }
    }
  }
}

}  // namespace

// Not a maze but a cave: open areas of irregular shape between blocks of solid rock.
// The grid only stores walls, so which cells are alive is kept here until the last step.
void CellularAutomatonStrategy::generate(
  MazeGrid & grid, Random & rng, const MazeStrategyOptions & options)
{
  const int width = grid.width();
  const int height = grid.height();
  Cells alive(grid.cell_count(), 0);
  initialize(alive, rng, initial_dead_probability(options.cave_level));
  apply_rules(alive, width, height, kIterations);
  ensure_connectivity(alive, width, height);
  carve_cave(grid, alive);
}

}  // namespace maze_gen
