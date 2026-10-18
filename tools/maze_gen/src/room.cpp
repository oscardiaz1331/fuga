#include "room.hpp"

namespace maze_gen
{

Room::Room(int x, int y, int width, int height)
: x_(x), y_(y), width_(width), height_(height)
{
}

bool Room::contains(int px, int py) const
{
  return px >= x_ && px < x_ + width_ && py >= y_ && py < y_ + height_;
}

bool Room::overlaps(const Room & other, int buffer) const
{
  return !(x_ >= other.x_ + other.width_ + buffer ||
         other.x_ >= x_ + width_ + buffer ||
         y_ >= other.y_ + other.height_ + buffer ||
         other.y_ >= y_ + height_ + buffer);
}

std::vector<std::pair<int, int>> Room::perimeter_cells() const
{
  std::vector<std::pair<int, int>> perimeter;
  for (int x = x_; x < x_ + width_; ++x) {
    perimeter.emplace_back(x, y_);
    if (height_ > 1) {
      perimeter.emplace_back(x, y_ + height_ - 1);
    }
  }
  // The side columns, without the corners already added
  for (int y = y_ + 1; y < y_ + height_ - 1; ++y) {
    perimeter.emplace_back(x_, y);
    if (width_ > 1) {
      perimeter.emplace_back(x_ + width_ - 1, y);
    }
  }
  return perimeter;
}

}  // namespace maze_gen
