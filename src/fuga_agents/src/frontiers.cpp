#include "fuga_agents/frontiers.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <queue>

namespace fuga_agents
{
namespace
{
constexpr int8_t kFree = 0;
constexpr int8_t kUnknown = -1;

// Cualquier valor que no sea libre ni desconocido, igual que en has_clearance
bool is_occupied(int8_t value)
{
  return value != kFree && value != kUnknown;
}

bool is_frontier_cell(
  int x, int y, int width, int height,
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

// true si no hay ninguna celda ocupada (cualquier valor que no sea libre ni
// desconocido) a clearance celdas o menos de (x, y), en distancia euclidea. Lo desconocido no cuenta: junto a una frontera
// siempre lo hay.
bool has_clearance(
  int x, int y, int width, int height,
  std::span<const int8_t> grid, int clearance)
{
  for (int dy = -clearance; dy <= clearance; ++dy) {
    for (int dx = -clearance; dx <= clearance; ++dx) {
      const int nx = x + dx;
      const int ny = y + dy;
      if (nx < 0 || nx >= width || ny < 0 || ny >= height ||
        dx * dx + dy * dy > clearance * clearance)
      {
        continue;
      }
      if (is_occupied(grid[ny * width + nx])) {
        return false;
      }
    }
  }
  return true;
}

// Fija como objetivo la celda del grupo con holgura mas cercana al centroide. Es
// una celda del grupo, y no el centroide, para que siempre sea libre (el centroide
// de una frontera curva puede caer en una pared). Devuelve false si ninguna celda
// tiene holgura.
bool set_target(
  Frontier & frontier, int width, int height,
  std::span<const int8_t> grid, int clearance)
{
  double cx = 0.0;
  double cy = 0.0;
  for (const int index : frontier.cells) {
    cx += index % width;
    cy += index / width;
  }
  cx /= frontier.cells.size();
  cy /= frontier.cells.size();

  bool found = false;
  double best = std::numeric_limits<double>::max();
  for (const int index : frontier.cells) {
    const int x = index % width;
    const int y = index / width;
    const double d = std::hypot(x - cx, y - cy);
    if (d < best && has_clearance(x, y, width, height, grid, clearance)) {
      best = d;
      frontier.target_x = x;
      frontier.target_y = y;
      found = true;
    }
  }
  return found;
}
} // namespace

std::vector<Frontier> find_frontiers(
  int width, int height,
  std::span<const int8_t> grid,
  int min_size, int clearance)
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

      if (static_cast<int>(frontier.cells.size()) >= min_size &&
        set_target(frontier, width, height, grid, clearance))
      {
        frontiers.push_back(std::move(frontier));
      }
    }
  }
  return frontiers;
}

bool has_frontier_near(
  int width, int height, std::span<const int8_t> grid,
  int cx, int cy, int radius)
{
  if (cx < 0 || cx >= width || cy < 0 || cy >= height) {
    return false;
  }
  const int x_min = std::max(0, cx - radius);
  const int x_max = std::min(width - 1, cx + radius);
  const int y_min = std::max(0, cy - radius);
  const int y_max = std::min(height - 1, cy + radius);
  for (int y = y_min; y <= y_max; ++y) {
    for (int x = x_min; x <= x_max; ++x) {
      if (is_frontier_cell(x, y, width, height, grid)) {
        return true;
      }
    }
  }
  return false;
}

std::vector<int> path_distances(
  int width, int height, std::span<const int8_t> grid,
  int start_x, int start_y)
{
  std::vector<int> distances(grid.size(), -1);
  if (start_x < 0 || start_x >= width || start_y < 0 || start_y >= height) {
    return distances;
  }

  std::queue<int> queue;
  const int start = start_y * width + start_x;
  distances[start] = 0;
  queue.push(start);

  constexpr int kDx[] = {1, -1, 0, 0};
  constexpr int kDy[] = {0, 0, 1, -1};
  while (!queue.empty()) {
    const int current = queue.front();
    queue.pop();
    const int cx = current % width;
    const int cy = current / width;
    for (int i = 0; i < 4; ++i) {
      const int nx = cx + kDx[i];
      const int ny = cy + kDy[i];
      if (nx < 0 || nx >= width || ny < 0 || ny >= height) {
        continue;
      }
      const int neighbor = ny * width + nx;
      if (distances[neighbor] == -1 && grid[neighbor] == kFree) {
        distances[neighbor] = distances[current] + 1;
        queue.push(neighbor);
      }
    }
  }
  return distances;
}

std::optional<Frontier> choose_frontier(
  const std::vector<Frontier> & frontiers,
  std::span<const int> distances, int width,
  double distance_weight,
  double size_weight)
{
  std::optional<Frontier> best;
  double best_cost = std::numeric_limits<double>::max();
  for (const Frontier & frontier : frontiers) {
    const int distance = distances[frontier.target_y * width + frontier.target_x];
    if (distance < 0) {
      continue;  // sin camino desde el robot
    }
    const double cost = distance_weight * distance -
      size_weight * static_cast<double>(frontier.cells.size());
    if (cost < best_cost) {
      best_cost = cost;
      best = frontier;
    }
  }
  return best;
}

std::vector<ExitCandidate> find_exits(
  int width, int height, std::span<const int8_t> grid,
  int min_size)
{
  // Caja de las celdas ocupadas: su borde es el muro exterior
  int x_min = width, x_max = -1, y_min = height, y_max = -1;
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if (is_occupied(grid[y * width + x])) {
        x_min = std::min(x_min, x);
        x_max = std::max(x_max, x);
        y_min = std::min(y_min, y);
        y_max = std::max(y_max, y);
      }
    }
  }
  if (x_max < 0) {
    return {};  // Sin muros no hay laberinto del que salir
  }
  const auto free_outside = [&](int x, int y) {
      return grid[y * width + x] == kFree &&
             (x<x_min || x> x_max || y<y_min || y> y_max);
    };

  std::vector<ExitCandidate> exits;
  std::vector<bool> visited(grid.size(), false);
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const int start = y * width + x;
      if (visited[start] || !free_outside(x, y)) {
        continue;
      }

      // Celdas del grupo pegadas a cada lado de la caja: suma de la coordenada a lo
      // largo del lado (para la media) y cuantas son
      struct Side
      {
        int out_dx, out_dy;
        long sum = 0;
        int count = 0;
      };
      std::array<Side, 4> sides{{{0, -1}, {0, 1}, {-1, 0}, {1, 0}}};  // S, N, O, E
      int size = 0;

      std::queue<int> queue;
      queue.push(start);
      visited[start] = true;
      while (!queue.empty()) {
        const int current = queue.front();
        queue.pop();
        ++size;
        const int cx = current % width;
        const int cy = current / width;
        const bool in_x = cx >= x_min && cx <= x_max;
        const bool in_y = cy >= y_min && cy <= y_max;
        if (in_x && cy == y_min - 1) {sides[0].sum += cx; ++sides[0].count;}
        if (in_x && cy == y_max + 1) {sides[1].sum += cx; ++sides[1].count;}
        if (in_y && cx == x_min - 1) {sides[2].sum += cy; ++sides[2].count;}
        if (in_y && cx == x_max + 1) {sides[3].sum += cy; ++sides[3].count;}

        for (int dy = -1; dy <= 1; ++dy) {
          for (int dx = -1; dx <= 1; ++dx) {
            const int nx = cx + dx;
            const int ny = cy + dy;
            if ((dx == 0 && dy == 0) || nx < 0 || nx >= width || ny < 0 || ny >= height) {
              continue;
            }
            const int neighbor = ny * width + nx;
            if (!visited[neighbor] && free_outside(nx, ny)) {
              visited[neighbor] = true;
              queue.push(neighbor);
            }
          }
        }
      }

      // El lado por el que sale el grupo es el que mas celdas toca. Si no toca
      // ninguno, son celdas sueltas lejos de la caja: no se sabe por donde se sale
      const Side & side = *std::max_element(
        sides.begin(), sides.end(),
        [](const Side & a, const Side & b) {return a.count < b.count;});
      if (size < min_size || side.count == 0) {
        continue;
      }
      ExitCandidate exit;
      exit.out_dx = side.out_dx;
      exit.out_dy = side.out_dy;
      exit.size = size;
      const int middle = static_cast<int>(std::lround(
          static_cast<double>(side.sum) / side.count));
      if (side.out_dy != 0) {
        exit.door_x = middle;
        exit.door_y = side.out_dy < 0 ? y_min - 1 : y_max + 1;
      } else {
        exit.door_y = middle;
        exit.door_x = side.out_dx < 0 ? x_min - 1 : x_max + 1;
      }
      exits.push_back(exit);
    }
  }
  return exits;
}
} // namespace fuga_agents
