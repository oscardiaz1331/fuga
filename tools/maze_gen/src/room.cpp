#include "maze_gen/room.hpp"

namespace maze_gen {

Room::Room(int x, int y, int w, int h)
    : _x(x), _y(y), _width(w), _height(h) {
}

bool Room::contains(int px, int py) const
{
    return px >= _x && px < _x + _width && py >= _y && py < _y + _height;
}

bool Room::overlaps(const Room& other, int buffer) const
{
    return !(_x >= other._x + other._width + buffer ||
        other._x >= _x + _width + buffer ||
        _y >= other._y + other._height + buffer ||
        other._y >= _y + _height + buffer);
}

std::pair<int, int> Room::center() const
{
    return { _x + _width / 2, _y + _height / 2 };
}

std::pair<int, int> Room::topLeft() const
{
    return { _x, _y };
}

int Room::height() const
{
    return _height;
}

int Room::width() const
{
    return _width;
}

std::vector<std::pair<int, int>> Room::getAllCells()
{
    std::vector<std::pair<int, int>> cells;
    for (int ry = _y; ry < _y + _height; ++ry) {
        for (int rx = _x; rx < _x + _width; ++rx) {
            cells.emplace_back(rx, ry);
        }
    }
    return cells;
}

std::vector<std::pair<int, int>> Room::getPerimeterCells() const
{
    std::vector<std::pair<int, int>> perimeter;

    // Top and bottom walls
    for (int rx = _x; rx < _x + _width; ++rx) {
        perimeter.emplace_back(rx, _y);              // Top
        perimeter.emplace_back(rx, _y + _height - 1); // Bottom
    }

    // Left and right walls (excluding corners already added)
    for (int ry = _y + 1; ry < _y + _height - 1; ++ry) {
        perimeter.emplace_back(_x, ry);              // Left
        perimeter.emplace_back(_x + _width - 1, ry);  // Right
    }

    return perimeter;
}

}  // namespace maze_gen
