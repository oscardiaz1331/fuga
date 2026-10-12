#pragma once
#include "maze_gen/maze_grid.hpp"
#include <vector>
#include <set>
#include <queue>
#include <unordered_set>
#include <unordered_map>

namespace maze_gen {

class MazeResolver {
public:
    // Check if all exits are connected to each other
    bool areAllExitsConnected(MazeGrid& grid);

    // Connect all exits by creating paths between disconnected components
    void connectAllExits(MazeGrid& grid);

    // Get connected components of exits (for debugging/analysis)
    std::vector<std::vector<int>> getExitComponents(MazeGrid& grid);

private:
    // Find all cells reachable from a starting position using BFS
    std::unordered_set<int> getReachableCells(MazeGrid& grid, int startX, int startY);

    // Convert 2D coordinates to 1D index for hashing
    int coordToIndex(int x, int y, int width) const;

    // Convert 1D index back to 2D coordinates
    std::pair<int, int> indexToCoord(int index, int width) const;

    // Check if there's a wall between two adjacent cells
    bool hasWallBetween(MazeGrid& grid, int x1, int y1, int x2, int y2) const;

    // Find shortest path between two points (A* algorithm)
    std::vector<std::pair<int, int>> findPath(MazeGrid& grid,
        int startX, int startY,
        int endX, int endY);

    // Carve a path along the given coordinates
    void carvePath(MazeGrid& grid, const std::vector<std::pair<int, int>>& path);

    // Get the actual exit position (the cell inside the maze, not outside)
    std::pair<int, int> getExitPosition(MazeGrid& grid, int x, int y, Dir d);

    // Manhattan distance heuristic for A*
    int manhattanDistance(int x1, int y1, int x2, int y2) const;
};

}  // namespace maze_gen
