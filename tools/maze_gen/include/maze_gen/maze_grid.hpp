#pragma once
#include <vector>
#include <array>
#include <cstdint>
#include <optional>
#include <iostream>
#include "maze_gen/cell.hpp"
#include <variant>

namespace maze_gen {

class MazeGrid {
public:
    using CellVariant = std::variant<Cell, VisitedCell, AliveCell>;
    MazeGrid(int w, int h)
        : _w(w), _h(h), _cells(static_cast<size_t>(w* h), Cell{}) {
    }
    int width()  const { return _w; }
    int height() const { return _h; }
    Cell& getCell(int x, int y);
    const Cell& getConstCell(int x, int y);
    VisitedCell& getVisitedCell(int x, int y);
    const VisitedCell& getConstVisitedCell(int x, int y);
    AliveCell& getAliveCell(int x, int y);
    const AliveCell& getConstAliveCell(int x, int y);
    bool in_bounds(int x, int y) const {
        return x >= 0 && y >= 0 && x < _w && y < _h;
    }
    const std::vector<std::tuple<int, int, Dir>>& getExits() const;
    void addExit(int x, int y, Dir d);
    // Carve a passage from (x,y) to direction d (and the opposite wall in the neighbor)
    bool carve(int x, int y, Dir d);
    std::optional<std::pair<int, int>> try_neighbor(int x, int y, Dir d) const;
    std::array<Dir, 4> directions4() const {
        return { Dir::N, Dir::E, Dir::S, Dir::W };
    }
    std::array<Dir, 8> directions8() const {
        return { Dir::N, Dir::NE, Dir::E, Dir::SE, Dir::S, Dir::SW, Dir::W, Dir::NW };
    }
    // Returns valid 4-neighbors (coords + dir from (x,y) to them)
    std::vector<std::tuple<int, int, Dir>> neighbors4(int x, int y) const;
    std::vector<std::tuple<int, int, Dir>> neighbors8(int x, int y) const;
    void move(int& x, int& y, Dir d) const;
    void reset(CellType type = CellType::Cell);
    void displaySimple(std::ostream& os) const;
    void displayComplex(std::ostream& os) const;
    static std::pair<int, int> neighbor(int x, int y, Dir d);
    CellVariant& at(int x, int y) { return _cells[index(x, y)]; }
    const CellVariant& at(int x, int y) const { return _cells[index(x, y)]; }
    uint8_t getWalls(const CellVariant& cellVariant) const {
        return std::visit([](const auto& cell) -> uint8_t {
            return cell.walls;
            }, cellVariant);
	}
private:
    CellType _cell_type = CellType::Cell;
    int _w, _h;
    std::vector<CellVariant> _cells;
    std::vector<std::tuple<int, int, Dir>> _exits;
    
    size_t index(int x, int y) const { return static_cast<size_t>(y * _w + x); }
    
};

// Free helper functions to work with cell variants
inline uint8_t getWalls(const MazeGrid::CellVariant& cellVariant) {
    return std::visit([](const auto& cell) -> uint8_t {
        return cell.walls;
        }, cellVariant);
}

inline void setWalls(MazeGrid::CellVariant& cellVariant, uint8_t walls) {
    std::visit([walls](auto& cell) {
        cell.walls = walls;
        }, cellVariant);
}

inline void removeWalls(MazeGrid::CellVariant& cellVariant, uint8_t wallsToRemove) {
    std::visit([wallsToRemove](auto& cell) {
        cell.walls &= ~wallsToRemove;
        }, cellVariant);
}

}  // namespace maze_gen
