#include "fuga_agents/frontiers.hpp"

#include <cmath>
#include <limits>
#include <queue>

namespace fuga_agents
{
namespace
{
constexpr int8_t kFree = 0;
constexpr int8_t kUnknown = -1;

bool is_frontier_cell(int x, int y, int width, int height,
                      std::span<const int8_t> grid)
{
  const int index = y * width + x;
  if (grid[index] != kFree) {
    return false;
  }
  return (x > 0 && grid[index - 1] == kUnknown) ||
         (x < width - 1 && grid[index + 1] == kUnknown) ||
         (y > 0 && grid[index - width] == kUnknown) ||
         (y < height - 1 && grid[index + width] == kUnknown);
}

// Fija como objetivo la celda del grupo mas cercana al centroide, para que
// siempre sea una celda libre (el centroide de una frontera curva puede caer en una pared).
void set_target(Frontier & frontier, int width)
{
  double cx = 0.0;
  double cy = 0.0;
  for (const int index : frontier.cells) {
    cx += index % width;
    cy += index / width;
  }
  cx /= frontier.cells.size();
  cy /= frontier.cells.size();

  double best = std::numeric_limits<double>::max();
  for (const int index : frontier.cells) {
    const double d = std::hypot(index % width - cx, index / width - cy);
    if (d < best) {
      best = d;
      frontier.target_x = index % width;
      frontier.target_y = index / width;
    }
  }
}
} // namespace

std::vector<Frontier> find_frontiers(int width, int height,
                                     std::span<const int8_t> grid,
                                     int min_size)
{
  std::vector<Frontier> frontiers;
  std::vector<bool> visited(grid.size(), false);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const int start = y * width + x;
      if (visited[start] || !is_frontier_cell(x, y, width, height, grid)) {
        continue;
      }

      // BFS desde esta celda: recoge todas las celdas frontera conectadas.
      Frontier frontier;
      std::queue<int> queue;
      queue.push(start);
      visited[start] = true;

      while (!queue.empty()) {
        const int current = queue.front();
        queue.pop();
        frontier.cells.push_back(current);

        const int cx = current % width;
        const int cy = current / width;
        for (int dy = -1; dy <= 1; ++dy) {
          for (int dx = -1; dx <= 1; ++dx) {
            const int nx = cx + dx;
            const int ny = cy + dy;
            if ((dx == 0 && dy == 0) || nx < 0 || nx >= width || ny < 0 ||
                ny >= height)
            {
              continue;
            }
            const int neighbor = ny * width + nx;
            if (!visited[neighbor] &&
                is_frontier_cell(nx, ny, width, height, grid))
            {
              visited[neighbor] = true;
              queue.push(neighbor);
            }
          }
        }
      }

      if (static_cast<int>(frontier.cells.size()) >= min_size) {
        set_target(frontier, width);
        frontiers.push_back(std::move(frontier));
      }
    }
  }
  return frontiers;
}

std::optional<Frontier> choose_frontier(const std::vector<Frontier> & frontiers,
                                        int robot_x, int robot_y,
                                        double distance_weight,
                                        double size_weight)
{
  std::optional<Frontier> best;
  double best_cost = std::numeric_limits<double>::max();
  for (const Frontier & frontier : frontiers) {
    const double distance = std::hypot(frontier.target_x - robot_x,
                                       frontier.target_y - robot_y);
    const double cost = distance_weight * distance -
                        size_weight * static_cast<double>(frontier.cells.size());
    if (cost < best_cost) {
      best_cost = cost;
      best = frontier;
    }
  }
  return best;
}
} // namespace fuga_agents
