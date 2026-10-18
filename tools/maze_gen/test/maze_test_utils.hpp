#pragma once
// Helpers shared by the maze_gen tests: structural measurements of a maze and a few
// shortcuts to build one.
#include <cstdint>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>
#include "algorithm_registry.hpp"
#include "maze_gen/maze_algorithm.hpp"
#include "maze_gen/maze_grid.hpp"
#include "maze_gen/maze_strategy_options.hpp"
#include "random.hpp"

namespace maze_gen
{

// Found by gtest through argument-dependent lookup, so failures read like
// "hunt_and_kill" and "(3, 0, N)" instead of raw bytes
inline void PrintTo(MazeAlgorithm algorithm, std::ostream * os)
{
  *os << to_string(algorithm);
}

inline void PrintTo(Dir dir, std::ostream * os)
{
  switch (dir) {
    case Dir::N: *os << "N"; return;
    case Dir::E: *os << "E"; return;
    case Dir::S: *os << "S"; return;
    case Dir::W: *os << "W"; return;
  }
  *os << "Dir(" << static_cast<int>(dir) << ")";
}

inline void PrintTo(const Exit & exit, std::ostream * os)
{
  *os << "(" << exit.x << ", " << exit.y << ", ";
  PrintTo(exit.dir, os);
  *os << ")";
}

namespace test
{

inline std::vector<MazeAlgorithm> every_algorithm()
{
  const auto all = all_algorithms();
  return {all.begin(), all.end()};
}

// The ones that must produce a perfect maze: all but the cave generator
inline std::vector<MazeAlgorithm> perfect_algorithms()
{
  std::vector<MazeAlgorithm> result;
  for (MazeAlgorithm algorithm : all_algorithms()) {
    if (algorithm_info(algorithm).perfect) {
      result.push_back(algorithm);
    }
  }
  return result;
}

inline std::string algorithm_name(MazeAlgorithm algorithm)
{
  return std::string(to_string(algorithm));
}

inline std::string size_name(int width, int height)
{
  return std::to_string(width) + "x" + std::to_string(height);
}

struct MazeStats
{
  int cells = 0;
  int regions = 0;          // groups of 2+ cells joined by open passages
  int closed_cells = 0;     // cells with no passage to any neighbor
  int passages = 0;         // open walls between two cells, each one counted once
  int border_openings = 0;  // open walls that lead outside the grid
  int asymmetric_walls = 0;  // walls open on one side and closed on the other
  int dead_ends = 0;        // cells with exactly one passage
};

inline MazeStats analyze(const MazeGrid & grid)
{
  MazeStats stats;
  stats.cells = grid.cell_count();

  std::vector<int> openings(stats.cells, 0);
  for (int y = 0; y < grid.height(); ++y) {
    for (int x = 0; x < grid.width(); ++x) {
      const int cell = grid.cell_index(x, y);
      for (Dir d : kDirections) {
        const auto [nx, ny] = MazeGrid::neighbor(x, y, d);
        const bool open = !grid.has_wall(x, y, d);
        if (!grid.in_bounds(nx, ny)) {
          if (open) {
            ++stats.border_openings;
          }
          continue;
        }
        if (open != !grid.has_wall(nx, ny, opposite(d))) {
          ++stats.asymmetric_walls;
        }
        if (!open) {
          continue;
        }
        ++openings[cell];
        // Count each passage from one of its two ends only
        if (d == Dir::E || d == Dir::S) {
          ++stats.passages;
        }
      }
      if (openings[cell] == 0) {
        ++stats.closed_cells;
      }
      if (openings[cell] == 1) {
        ++stats.dead_ends;
      }
    }
  }

  // Flood fill over the cells that have at least one passage
  std::vector<char> seen(stats.cells, 0);
  std::vector<int> queue;
  for (int start = 0; start < stats.cells; ++start) {
    if (seen[start] || openings[start] == 0) {
      continue;
    }
    ++stats.regions;
    queue.assign(1, start);
    seen[start] = 1;
    for (size_t head = 0; head < queue.size(); ++head) {
      const int x = queue[head] % grid.width();
      const int y = queue[head] / grid.width();
      for (const auto & [nx, ny, dir] : grid.neighbors(x, y)) {
        const int neighbor = grid.cell_index(nx, ny);
        if (!grid.has_wall(x, y, dir) && !seen[neighbor]) {
          seen[neighbor] = 1;
          queue.push_back(neighbor);
        }
      }
    }
  }
  return stats;
}

// One path between any two cells: connected, no loops, no half-open walls.
// A single cell is the trivial perfect maze.
inline bool is_perfect(const MazeStats & stats)
{
  if (stats.asymmetric_walls != 0 || stats.passages != stats.cells - 1) {
    return false;
  }
  if (stats.cells == 1) {
    return true;
  }
  return stats.regions == 1 && stats.closed_cells == 0;
}

// The wall mask of every cell, row by row: two mazes are the same if these are equal
inline std::vector<uint8_t> walls_of(const MazeGrid & grid)
{
  std::vector<uint8_t> walls;
  walls.reserve(grid.cell_count());
  for (int y = 0; y < grid.height(); ++y) {
    for (int x = 0; x < grid.width(); ++x) {
      walls.push_back(grid.walls(x, y));
    }
  }
  return walls;
}

// The same, as text: one hexadecimal digit per cell
inline std::string walls_text(const MazeGrid & grid)
{
  std::string text;
  for (uint8_t walls : walls_of(grid)) {
    text += "0123456789abcdef"[walls & 0x0F];
  }
  return text;
}

// Run only the algorithm on a fresh grid: no braiding, rooms, exits or connector
inline MazeGrid run_strategy(
  MazeAlgorithm algorithm, int width, int height, std::optional<uint64_t> seed,
  const MazeStrategyOptions & options = {})
{
  MazeGrid grid(width, height);
  Random rng(seed);
  algorithm_info(algorithm).create()->generate(grid, rng, options);
  return grid;
}

// The text format MazeGrid(path) reads. Only the tests save mazes
inline std::string serialize(const MazeGrid & grid)
{
  std::ostringstream os;
  os << grid.width() << ' ' << grid.height() << '\n' << std::hex;
  for (int y = 0; y < grid.height(); ++y) {
    for (int x = 0; x < grid.width(); ++x) {
      os << static_cast<int>(grid.walls(x, y));
    }
    os << '\n';
  }
  os << std::dec;
  for (const Exit & e : grid.exits()) {
    const char dir = e.dir == Dir::N ? 'N' : e.dir == Dir::E ? 'E' : e.dir == Dir::S ? 'S' : 'W';
    os << "exit " << e.x << ' ' << e.y << ' ' << dir << '\n';
  }
  return os.str();
}

}  // namespace test
}  // namespace maze_gen
