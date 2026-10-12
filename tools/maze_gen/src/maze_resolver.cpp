#include "maze_gen/maze_resolver.hpp"
#include <algorithm>
#include <climits>

namespace maze_gen {

bool MazeResolver::areAllExitsConnected(MazeGrid& grid) {
    auto exits = grid.getExits();
    if (exits.size() <= 1) {
        return true; // 0 or 1 exit is always "connected"
    }

    // Get the position of the first exit
    auto [firstX, firstY, firstDir] = exits[0];
    auto [exitX, exitY] = getExitPosition(grid, firstX, firstY, firstDir);

    // Find all reachable cells from the first exit
    auto reachable = getReachableCells(grid, exitX, exitY);

    // Check if all other exits are reachable
    for (size_t i = 1; i < exits.size(); ++i) {
        auto [x, y, dir] = exits[i];
        auto [checkX, checkY] = getExitPosition(grid, x, y, dir);
        int index = coordToIndex(checkX, checkY, grid.width());

        if (reachable.find(index) == reachable.end()) {
            return false; // This exit is not reachable
        }
    }

    return true;
}

void MazeResolver::connectAllExits(MazeGrid& grid) {
    auto components = getExitComponents(grid);

    if (components.size() <= 1) {
        return; // All exits already connected
    }

    // Connect each component to the first component
    auto exits = grid.getExits();

    for (size_t i = 1; i < components.size(); ++i) {
        // Get representative exit from first component
        int firstExitIdx = components[0][0];
        auto [x1, y1, d1] = exits[firstExitIdx];
        auto [startX, startY] = getExitPosition(grid, x1, y1, d1);

        // Get representative exit from current component
        int currentExitIdx = components[i][0];
        auto [x2, y2, d2] = exits[currentExitIdx];
        auto [endX, endY] = getExitPosition(grid, x2, y2, d2);

        // Find and carve path between them
        auto path = findPath(grid, startX, startY, endX, endY);
        if (!path.empty()) {
            carvePath(grid, path);
        }
    }
}

std::vector<std::vector<int>> MazeResolver::getExitComponents(MazeGrid& grid) {
    auto exits = grid.getExits();
    std::vector<std::vector<int>> components;
    std::vector<bool> visited(exits.size(), false);

    for (size_t i = 0; i < exits.size(); ++i) {
        if (visited[i]) continue;

        // Start new component
        std::vector<int> component;
        std::queue<int> queue;
        queue.push(i);
        visited[i] = true;

        while (!queue.empty()) {
            int currentIdx = queue.front();
            queue.pop();
            component.push_back(currentIdx);

            auto [currentX, currentY, currentDir] = exits[currentIdx];
            auto [posX, posY] = getExitPosition(grid, currentX, currentY, currentDir);
            auto reachable = getReachableCells(grid, posX, posY);

            // Check which other exits are reachable from this one
            for (size_t j = 0; j < exits.size(); ++j) {
                if (visited[j]) continue;

                auto [x, y, dir] = exits[j];
                auto [checkX, checkY] = getExitPosition(grid, x, y, dir);
                int index = coordToIndex(checkX, checkY, grid.width());

                if (reachable.find(index) != reachable.end()) {
                    visited[j] = true;
                    queue.push(j);
                }
            }
        }

        components.push_back(component);
    }

    return components;
}

std::unordered_set<int> MazeResolver::getReachableCells(MazeGrid& grid, int startX, int startY) {
    std::unordered_set<int> visited;
    std::queue<std::pair<int, int>> queue;

    queue.push({ startX, startY });
    visited.insert(coordToIndex(startX, startY, grid.width()));

    while (!queue.empty()) {
        auto [x, y] = queue.front();
        queue.pop();

        // Check all 4 directions
        for (auto dir : { Dir::N, Dir::S, Dir::E, Dir::W }) {
            auto [nx, ny] = grid.neighbor(x, y, dir);

            // Skip if out of bounds
            if (!grid.in_bounds(nx, ny)) continue;

            int neighborIndex = coordToIndex(nx, ny, grid.width());

            // Skip if already visited
            if (visited.find(neighborIndex) != visited.end()) continue;

            // Skip if there's a wall between current cell and neighbor
            if (hasWallBetween(grid, x, y, nx, ny)) continue;

            visited.insert(neighborIndex);
            queue.push({ nx, ny });
        }
    }

    return visited;
}

int MazeResolver::coordToIndex(int x, int y, int width) const {
    return y * width + x;
}

std::pair<int, int> MazeResolver::indexToCoord(int index, int width) const {
    return { index % width, index / width };
}

bool MazeResolver::hasWallBetween(MazeGrid& grid, int x1, int y1, int x2, int y2) const {
    // Determine direction from (x1,y1) to (x2,y2)
    Dir direction;
    if (x2 == x1 + 1 && y2 == y1) direction = Dir::E;
    else if (x2 == x1 - 1 && y2 == y1) direction = Dir::W;
    else if (x2 == x1 && y2 == y1 + 1) direction = Dir::S;
    else if (x2 == x1 && y2 == y1 - 1) direction = Dir::N;
    else return true; // Not adjacent cells

    // Check if there's a wall in that direction
    const auto& cell = grid.at(x1, y1);
    uint8_t walls = grid.getWalls(cell);
    return (walls & static_cast<uint8_t>(direction)) != 0;
}

std::vector<std::pair<int, int>> MazeResolver::findPath(MazeGrid& grid,
    int startX, int startY,
    int endX, int endY) {
    struct Node {
        int x, y;
        int gCost, hCost;
        int parentIndex;

        int fCost() const { return gCost + hCost; }
    };

    std::vector<Node> nodes;
    std::unordered_map<int, int> coordToNodeIndex;
    std::set<int> openSet;
    std::unordered_set<int> closedSet;

    // Create start node
    Node startNode = { startX, startY, 0, manhattanDistance(startX, startY, endX, endY), -1 };
    nodes.push_back(startNode);
    int startIndex = 0;
    coordToNodeIndex[coordToIndex(startX, startY, grid.width())] = startIndex;
    openSet.insert(startIndex);

    while (!openSet.empty()) {
        // Find node with lowest f cost
        int currentIndex = *std::min_element(openSet.begin(), openSet.end(),
            [&nodes](int a, int b) { return nodes[a].fCost() < nodes[b].fCost(); });

        openSet.erase(currentIndex);
        closedSet.insert(currentIndex);

        Node& current = nodes[currentIndex];

        // Check if we reached the goal
        if (current.x == endX && current.y == endY) {
            // Reconstruct path
            std::vector<std::pair<int, int>> path;
            int pathIndex = currentIndex;
            while (pathIndex != -1) {
                path.emplace_back(nodes[pathIndex].x, nodes[pathIndex].y);
                pathIndex = nodes[pathIndex].parentIndex;
            }
            std::reverse(path.begin(), path.end());
            return path;
        }

        // Check neighbors
        for (auto dir : { Dir::N, Dir::S, Dir::E, Dir::W }) {
            auto [nx, ny] = grid.neighbor(current.x, current.y, dir);

            if (!grid.in_bounds(nx, ny)) continue;

            int neighborCoord = coordToIndex(nx, ny, grid.width());

            // Skip if in closed set
            auto nodeIt = coordToNodeIndex.find(neighborCoord);
            if (nodeIt != coordToNodeIndex.end() &&
                closedSet.find(nodeIt->second) != closedSet.end()) {
                continue;
            }

            int tentativeGCost = current.gCost + 1;

            // Check if this neighbor is already in open set
            int neighborIndex = -1;
            if (nodeIt != coordToNodeIndex.end()) {
                neighborIndex = nodeIt->second;
                if (tentativeGCost >= nodes[neighborIndex].gCost) {
                    continue; // This path is not better
                }
            }
            else {
                // Create new node
                Node neighborNode = { nx, ny, tentativeGCost,
                                   manhattanDistance(nx, ny, endX, endY), currentIndex };
                nodes.push_back(neighborNode);
                neighborIndex = nodes.size() - 1;
                coordToNodeIndex[neighborCoord] = neighborIndex;
            }

            // Update node
            nodes[neighborIndex].gCost = tentativeGCost;
            nodes[neighborIndex].parentIndex = currentIndex;
            openSet.insert(neighborIndex);
        }
    }

    return {}; // No path found
}

void MazeResolver::carvePath(MazeGrid& grid, const std::vector<std::pair<int, int>>& path) {
    for (size_t i = 0; i < path.size() - 1; ++i) {
        auto [x1, y1] = path[i];
        auto [x2, y2] = path[i + 1];

        // Determine direction
        Dir direction;
        if (x2 == x1 + 1) direction = Dir::E;
        else if (x2 == x1 - 1) direction = Dir::W;
        else if (y2 == y1 + 1) direction = Dir::S;
        else if (y2 == y1 - 1) direction = Dir::N;
        else continue; // Skip non-adjacent cells

        // Carve the connection
        grid.carve(x1, y1, direction);
    }
}

std::pair<int, int> MazeResolver::getExitPosition(MazeGrid& grid, int x, int y, Dir d) {
    // The exit position is the cell inside the maze that the exit connects to
    // Since exits are on the perimeter, we need to move inward from the exit direction
    switch (d) {
    case Dir::N: return { x, y }; // Exit goes north, so the cell is at (x, y)
    case Dir::S: return { x, y }; // Exit goes south, so the cell is at (x, y)  
    case Dir::E: return { x, y }; // Exit goes east, so the cell is at (x, y)
    case Dir::W: return { x, y }; // Exit goes west, so the cell is at (x, y)
    default: return { x, y };
    }
}

int MazeResolver::manhattanDistance(int x1, int y1, int x2, int y2) const {
    return std::abs(x1 - x2) + std::abs(y1 - y2);
}

}  // namespace maze_gen
