#pragma once
#include <utility>
#include <vector>

namespace maze_gen
{

// A rectangle of cells
class Room
{
public:
  Room(int x, int y, int width, int height);

  std::pair<int, int> top_left() const {return {x_, y_};}
  int width() const {return width_;}
  int height() const {return height_;}

  bool contains(int px, int py) const;
  // True if both rooms share a cell or are closer than `buffer` cells to each other
  bool overlaps(const Room & other, int buffer = 1) const;
  // The cells on the edge of the room, each one once
  std::vector<std::pair<int, int>> perimeter_cells() const;

private:
  int x_;
  int y_;
  int width_;
  int height_;
};

}  // namespace maze_gen
