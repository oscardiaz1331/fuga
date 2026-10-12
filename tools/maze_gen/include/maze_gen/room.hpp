#pragma once
#include <utility>
#include <vector>

namespace maze_gen {

class Room
{
private:
    int _x, _y;           // Top-left corner
    int _width, _height;  // Room dimensions

public:
    Room(int x, int y, int w, int h);

    // Check if point is inside room
    bool contains(int px, int py) const;

    // Check if this room overlaps with another (including 1-cell buffer)
    bool overlaps(const Room& other, int buffer = 1) const;

    // Get center point of room
    std::pair<int, int> center() const;

	// Get top-left corner
	std::pair<int, int> topLeft() const;

	// Get Height
	int height() const;

    // Get Width
	int width() const;

    // Get all cells in room
    std::vector<std::pair<int, int>> getAllCells();

    // Get perimeter cells (for connecting to maze)
    std::vector<std::pair<int, int>> getPerimeterCells() const;
};

}  // namespace maze_gen
