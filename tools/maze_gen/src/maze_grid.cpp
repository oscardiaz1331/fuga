#include "maze_gen/maze_grid.hpp"
#include <algorithm>
#include <cassert>
#include <charconv>
#include <fstream>
#include <limits>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include "box_drawing_chars.hpp"

namespace maze_gen
{

namespace
{

// Checked before the cells are allocated, so a bad size never reaches the vector
size_t checked_cell_count(int width, int height)
{
  if (width < 1 || height < 1) {
    throw std::invalid_argument("MazeGrid: width and height must be at least 1");
  }
  // Cell indices are ints all over the library, so the count has to fit in one
  if (width > std::numeric_limits<int>::max() / height) {
    throw std::invalid_argument("MazeGrid: width * height is too large");
  }
  return static_cast<size_t>(width) * static_cast<size_t>(height);
}

uint8_t bit(Dir d)
{
  return static_cast<uint8_t>(d);
}

}  // namespace

MazeGrid::MazeGrid(int width, int height)
: width_(width), height_(height), walls_(checked_cell_count(width, height), kAllWalls)
{
}

MazeGrid::MazeGrid(const std::filesystem::path & path)
: MazeGrid(load(path))
{
}

MazeGrid MazeGrid::load(const std::filesystem::path & path)
{
  const auto fail = [&path](const std::string & what) {
      throw std::runtime_error("MazeGrid: " + path.string() + ": " + what);
    };

  std::ifstream file(path);
  if (!file) {
    fail("cannot open the file");
  }

  int width = 0, height = 0;
  if (!(file >> width >> height)) {
    fail("expected the size as \"width height\"");
  }
  std::optional<MazeGrid> grid;
  try {
    grid.emplace(width, height);
  } catch (const std::invalid_argument & e) {
    fail(e.what());
  }

  std::string row;
  for (int y = 0; y < height; ++y) {
    if (!(file >> row) || row.size() != static_cast<size_t>(width)) {
      fail("row " + std::to_string(y) + ": expected " + std::to_string(width) + " hex digits");
    }
    for (int x = 0; x < width; ++x) {
      uint8_t walls = 0;
      const auto [end, error] = std::from_chars(&row[x], &row[x] + 1, walls, 16);
      if (error != std::errc()) {
        fail("row " + std::to_string(y) + ": '" + row[x] + "' is not a hex digit");
      }
      grid->walls_[grid->cell_index(x, y)] = walls;
    }
  }

  std::string word;
  while (file >> word) {
    int x = 0, y = 0;
    char dir = 0;
    if (word != "exit" || !(file >> x >> y >> dir)) {
      fail("expected \"exit x y N|E|S|W\"");
    }
    const size_t i = std::string_view("NESW").find(dir);
    if (i == std::string_view::npos || !grid->add_exit(x, y, kDirections[i])) {
      fail(
        "exit " + std::to_string(x) + " " + std::to_string(y) + " " + dir +
        " is not on the outer wall or is repeated");
    }
  }
  return std::move(*grid);
}

uint8_t MazeGrid::walls(int x, int y) const
{
  assert(in_bounds(x, y));
  return walls_[cell_index(x, y)];
}

bool MazeGrid::has_wall(int x, int y, Dir d) const
{
  return (walls(x, y) & bit(d)) != 0;
}

std::pair<int, int> MazeGrid::neighbor(int x, int y, Dir d)
{
  switch (d) {
    case Dir::N: return {x, y - 1};
    case Dir::E: return {x + 1, y};
    case Dir::S: return {x, y + 1};
    case Dir::W: return {x - 1, y};
  }
  return {x, y};  // unreachable
}

NeighborList MazeGrid::neighbors(int x, int y) const
{
  NeighborList result;
  for (Dir d : kDirections) {
    const auto [nx, ny] = neighbor(x, y, d);
    if (in_bounds(nx, ny)) {
      result.push_back({nx, ny, d});
    }
  }
  return result;
}

bool MazeGrid::carve(int x, int y, Dir d)
{
  const auto [nx, ny] = neighbor(x, y, d);
  if (!in_bounds(x, y) || !in_bounds(nx, ny)) {
    return false;
  }
  walls_[cell_index(x, y)] &= static_cast<uint8_t>(~bit(d));
  walls_[cell_index(nx, ny)] &= static_cast<uint8_t>(~bit(opposite(d)));
  return true;
}

bool MazeGrid::add_wall(int x, int y, Dir d)
{
  const auto [nx, ny] = neighbor(x, y, d);
  if (!in_bounds(x, y) || !in_bounds(nx, ny)) {
    return false;
  }
  walls_[cell_index(x, y)] |= bit(d);
  walls_[cell_index(nx, ny)] |= bit(opposite(d));
  return true;
}

bool MazeGrid::add_exit(int x, int y, Dir d)
{
  const auto [nx, ny] = neighbor(x, y, d);
  if (!in_bounds(x, y) || in_bounds(nx, ny)) {
    return false;  // not a cell of the grid, or d does not point outside
  }
  const Exit exit{x, y, d};
  if (std::find(exits_.begin(), exits_.end(), exit) != exits_.end()) {
    return false;
  }
  walls_[cell_index(x, y)] &= static_cast<uint8_t>(~bit(d));
  exits_.push_back(exit);
  return true;
}

void MazeGrid::reset()
{
  std::fill(walls_.begin(), walls_.end(), kAllWalls);
  exits_.clear();
}

void MazeGrid::display_simple(std::ostream & os) const
{
  os << "+";
  for (int x = 0; x < width_; ++x) {
    os << (has_wall(x, 0, Dir::N) ? "---" : "   ") << "+";
  }
  os << "\n";
  for (int y = 0; y < height_; ++y) {
    os << (has_wall(0, y, Dir::W) ? "|" : " ");
    for (int x = 0; x < width_; ++x) {
      os << "   " << (has_wall(x, y, Dir::E) ? "|" : " ");
    }
    os << "\n";

    os << "+";
    for (int x = 0; x < width_; ++x) {
      os << (has_wall(x, y, Dir::S) ? "---" : "   ") << "+";
    }
    os << "\n";
  }
}

void MazeGrid::display_complex(std::ostream & os) const
{
  for (int y = 0; y < height_; ++y) {
    for (int x = 0; x < width_; ++x) {
      os << get_box_drawing_char(
        has_wall(x, y, Dir::N), has_wall(x, y, Dir::S),
        has_wall(x, y, Dir::E), has_wall(x, y, Dir::W));
    }
    os << '\n';
  }
}

}  // namespace maze_gen
