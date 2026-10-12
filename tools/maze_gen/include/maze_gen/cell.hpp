#pragma once
#include <cstdint>

namespace maze_gen {

enum class Dir : uint8_t {
    N = 1 << 0,  // North
    NE = 1 << 1, // Northeast
    E = 1 << 2,  // East
    SE = 1 << 3, // Southeast
    S = 1 << 4,  // South
    SW = 1 << 5, // Southwest
    W = 1 << 6,  // West
    NW = 1 << 7  // Northwest
};

inline Dir opposite(Dir d) {
    switch (d) {
    case Dir::N: return Dir::S;
    case Dir::S: return Dir::N;
    case Dir::E: return Dir::W;
    case Dir::W: return Dir::E;
    case Dir::NE: return Dir::SW;
    case Dir::NW: return Dir::SE;
    case Dir::SE: return Dir::NW;
    case Dir::SW: return Dir::NE;
    }
    return Dir::N; // unreachable
}

enum class CellType : uint8_t {
    Cell,
    VisitedCell,
    AliveCell
};

struct Cell {
    // A bit set means "wall exists". Start FULL and clear bits when carving.
    enum class Dir : uint8_t {
        N = 1 << 0,  // North
        NE = 1 << 1, // Northeast
        E = 1 << 2,  // East
        SE = 1 << 3, // Southeast
        S = 1 << 4,  // South
        SW = 1 << 5, // Southwest
        W = 1 << 6,  // West
        NW = 1 << 7  // Northwest
    };
    uint8_t walls = uint8_t(Dir::N) | uint8_t(Dir::NE) | uint8_t(Dir::E) |
        uint8_t(Dir::SE) | uint8_t(Dir::S) | uint8_t(Dir::SW) |
        uint8_t(Dir::W) | uint8_t(Dir::NW);
};

struct VisitedCell : public Cell {
    bool isVisited = false;
};

struct AliveCell : public Cell {
    bool isAlive = false;
};

}  // namespace maze_gen
