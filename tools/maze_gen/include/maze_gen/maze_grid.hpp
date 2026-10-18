#pragma once
#include <array>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iosfwd>
#include <utility>
#include <vector>
#include "maze_gen/direction.hpp"

namespace maze_gen
{

// An opening in the outer wall: the border cell it belongs to and the side it is on
struct Exit
{
  int x;
  int y;
  Dir dir;

  auto operator<=>(const Exit &) const = default;
};

// A cell next to another one, and the direction that leads to it
struct Neighbor
{
  int x;
  int y;
  Dir dir;
};

// Up to 4 neighbors stored inline, so asking for them never allocates
class NeighborList
{
public:
  void push_back(const Neighbor & n) {items_[count_++] = n;}
  size_t size() const {return count_;}
  bool empty() const {return count_ == 0;}
  Neighbor & operator[](size_t i) {return items_[i];}
  const Neighbor & operator[](size_t i) const {return items_[i];}
  Neighbor * begin() {return items_.data();}
  Neighbor * end() {return items_.data() + count_;}
  const Neighbor * begin() const {return items_.data();}
  const Neighbor * end() const {return items_.data() + count_;}

private:
  std::array<Neighbor, 4> items_{};
  size_t count_ = 0;
};

// A rectangle of cells with walls between them. (x, y) grows to the right and DOWN:
// north of a cell is y - 1. A new grid has every wall in place.
class MazeGrid
{
public:
  // Throws std::invalid_argument unless width >= 1 and height >= 1
  MazeGrid(int width, int height);
  // Load a maze saved as text: a "width height" line, one line per row with one hex digit
  // per cell (the Dir bits that are walls), then one "exit x y N|E|S|W" line per exit.
  // Throws std::runtime_error saying what is wrong if the file cannot be read or parsed
  explicit MazeGrid(const std::filesystem::path & path);

  int width() const {return width_;}
  int height() const {return height_;}
  int cell_count() const {return width_ * height_;}
  bool in_bounds(int x, int y) const {return x >= 0 && y >= 0 && x < width_ && y < height_;}
  // Position of a cell in row-major order: what the algorithms use to index their own data
  int cell_index(int x, int y) const {return y * width_ + x;}

  // Wall bits of a cell (see Dir). The cell has to be inside the grid
  uint8_t walls(int x, int y) const;
  bool has_wall(int x, int y, Dir d) const;

  // Remove the wall between (x, y) and its neighbor. Returns false, changing nothing,
  // if either of them is outside the grid: the outer wall is only opened by add_exit
  bool carve(int x, int y, Dir d);
  // Put the wall between (x, y) and its neighbor back
  bool add_wall(int x, int y, Dir d);

  // Open the outer wall of a border cell and record it as an exit. Returns false,
  // changing nothing, if d does not point outside the grid or the exit already exists
  bool add_exit(int x, int y, Dir d);
  const std::vector<Exit> & exits() const {return exits_;}

  // Coordinates of the cell next to (x, y), which may fall outside the grid
  static std::pair<int, int> neighbor(int x, int y, Dir d);
  // The neighbors of (x, y) that are inside the grid, in N, E, S, W order
  NeighborList neighbors(int x, int y) const;

  // Put every wall back and forget the exits
  void reset();

  // ASCII drawing, three characters per cell
  void display_simple(std::ostream & os) const;
  // One box-drawing character per cell, showing which of its walls are up
  void display_complex(std::ostream & os) const;

private:
  static MazeGrid load(const std::filesystem::path & path);

  int width_;
  int height_;
  std::vector<uint8_t> walls_;
  std::vector<Exit> exits_;
};

}  // namespace maze_gen
