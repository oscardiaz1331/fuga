#include "maze_gen/maze_grid.hpp"
#include "maze_gen/box_drawing_chars.hpp"
#include <tuple>

namespace maze_gen {

std::pair<int, int> MazeGrid::neighbor(int x, int y, Dir d)
{
    switch (d) {
        case Dir::N: return { x, y - 1 };
        case Dir::S: return { x, y + 1 };
        case Dir::E: return { x + 1, y };
        case Dir::W: return { x - 1, y };
		case Dir::NE: return { x + 1, y - 1 };
		case Dir::NW: return { x - 1, y - 1 };
		case Dir::SE: return { x + 1, y + 1 };
        case Dir::SW: return { x - 1, y + 1 };
	    default: return { x, y }; // Should not happen
    }
    return { x,y };
}

Cell& MazeGrid::getCell(int x, int y) {
    if (!std::holds_alternative<Cell>(_cells[y * _w + x])) {
        throw std::bad_variant_access();
    }
    return std::get<Cell>(_cells[y * _w + x]);
}

const Cell& MazeGrid::getConstCell(int x, int y) {
    if (!std::holds_alternative<Cell>(_cells[y * _w + x])) {
        throw std::bad_variant_access();
    }
    return std::get<Cell>(_cells[y * _w + x]);
}

VisitedCell& MazeGrid::getVisitedCell(int x, int y) {
    if (!std::holds_alternative<VisitedCell>(_cells[y * _w + x])) {
        throw std::bad_variant_access();
    }
    return std::get<VisitedCell>(_cells[y * _w + x]);
}

const VisitedCell& MazeGrid::getConstVisitedCell(int x, int y) {
    if (!std::holds_alternative<VisitedCell>(_cells[y * _w + x])) {
        throw std::bad_variant_access();
    }
    return std::get<VisitedCell>(_cells[y * _w + x]);
}

AliveCell& MazeGrid::getAliveCell(int x, int y) {
    if (!std::holds_alternative<AliveCell>(_cells[y * _w + x])) {
        throw std::bad_variant_access();
    }
    return std::get<AliveCell>(_cells[y * _w + x]);
}

const AliveCell& MazeGrid::getConstAliveCell(int x, int y) {
    if (!std::holds_alternative<AliveCell>(_cells[y * _w + x])) {
        throw std::bad_variant_access();
    }
    return std::get<AliveCell>(_cells[y * _w + x]);
}

const std::vector<std::tuple<int, int, Dir>>& MazeGrid::getExits() const
{
    return _exits;
}

void MazeGrid::addExit(int x, int y, Dir d)
{
    _exits.emplace_back(x, y, d);
}

bool MazeGrid::carve(int x, int y, Dir d)
{
    if (!in_bounds(x, y)) return false;
    auto [nx, ny] = neighbor(x, y, d);
    removeWalls(at(x, y), static_cast<uint8_t>(d));
    if (!in_bounds(nx, ny)) return false;
    removeWalls(at(nx, ny), static_cast<uint8_t>(opposite(d)));
    return true;
}

std::optional<std::pair<int, int>> MazeGrid::try_neighbor(int x, int y, Dir d) const
{
    auto [nx, ny] = neighbor(x, y, d);
    if (!in_bounds(nx, ny)) return std::nullopt;
    return std::make_pair(nx, ny);
}

std::vector<std::tuple<int, int, Dir>> MazeGrid::neighbors4(int x, int y) const
{
    std::vector<std::tuple<int, int, Dir>> res;
    res.reserve(4);
    for (auto d : directions4()) {
        auto n = try_neighbor(x, y, d);
        if (n) res.emplace_back(n->first, n->second, d);
    }
    return res;
}

std::vector<std::tuple<int, int, Dir>> MazeGrid::neighbors8(int x, int y) const
{
    std::vector<std::tuple<int, int, Dir>> res;
    res.reserve(8);
    for (auto d : directions8()) {
        auto n = try_neighbor(x, y, d);
        if (n) res.emplace_back(n->first, n->second, d);
    }
    return res;
}

void MazeGrid::move(int& x, int& y, Dir d) const
{
    switch (d) {
        case Dir::N:  --y;      break;
        case Dir::NE: ++x; --y; break;
        case Dir::E:  ++x;      break;
        case Dir::SE: ++x; ++y; break;
        case Dir::S:  ++y;      break;
        case Dir::SW: --x; ++y; break;
        case Dir::W:  --x;      break;
        case Dir::NW: --x; --y; break;
    }
}

void MazeGrid::reset(CellType type) {
    _cell_type = type;
    switch (_cell_type) {
    case CellType::Cell:
        for (auto& cell : _cells) {
            cell = Cell{};
        }
        break;
    case CellType::VisitedCell:
        for (auto& cell : _cells) {
            cell = VisitedCell{};
        }
        break;
    case CellType::AliveCell:
        for (auto& cell : _cells) {
            cell = AliveCell{};
        }
        break;
    default:
        for (auto& cell : _cells) {
            cell = Cell{};
        }
        break;
    }
}

void MazeGrid::displaySimple(std::ostream& os) const
{
    auto hasWall = [this](const CellVariant& c, Dir d) {
        return (getWalls(c) & static_cast<uint8_t>(d)) != 0;
        };
    os << "+";
    for (int x = 0; x < _w; ++x) {
        const auto& c = at(x, 0);
        os << (hasWall(c, Dir::N) ? "---" : "   ") << "+";
    }
    os << "\n";
    for (int y = 0; y < _h; ++y) {
        os << (hasWall(at(0, y), Dir::W) ? "|" : " ");
        for (int x = 0; x < _w; ++x) {
            os << "   ";
            os << (hasWall(at(x, y), Dir::E) ? "|" : " ");
        }
        os << "\n";

        os << "+";
        for (int x = 0; x < _w; ++x) {
            const auto& c = at(x, y);
            os << (hasWall(c, Dir::S) ? "---" : "   ") << "+";
        }
        os << "\n";
    }
}

void MazeGrid::displayComplex(std::ostream& os) const
{
    for (int y = 0; y < _h; ++y) {
        for (int x = 0; x < _w; ++x) {
            const auto& c = at(x, y);
            uint8_t walls = getWalls(c);

            bool n = (walls & static_cast<uint8_t>(Dir::N));
            bool s = (walls & static_cast<uint8_t>(Dir::S));
            bool e = (walls & static_cast<uint8_t>(Dir::E));
            bool w = (walls & static_cast<uint8_t>(Dir::W));

            bool nw = (walls & static_cast<uint8_t>(Dir::NW));
            bool ne = (walls & static_cast<uint8_t>(Dir::NE));
            bool sw = (walls & static_cast<uint8_t>(Dir::SW));
            bool se = (walls & static_cast<uint8_t>(Dir::SE));

            char diag = ' ';
            if ((nw || se) && (ne || sw)) diag = 'X';
            else if (nw || se) diag = '\\';
            else if (ne || sw) diag = '/';

            std::string_view box = getBoxDrawingChar(n, s, e, w);
            if (box != " ")
                os << box;
            else
                os << diag;
        }
        os << '\n';
    }
}

}  // namespace maze_gen
