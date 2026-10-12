#pragma once
#include "maze_gen/i_maze_generation_strategy.hpp"
#include <vector>
#include <tuple>
#include <unordered_set>

namespace maze_gen {

class PrimsStrategy : public IMazeGenerationStrategy {
public:
    void generateMaze(MazeGrid& grid) override;

private:
    using Edge = std::tuple<int, int, Dir>; // (x, y, direction to neighbor)
    std::vector<Edge> _edges;
    std::unordered_set<int> _visited;

    bool isInside(int x, int y, int width, int height);
    int toIndex(int x, int y, int width);
};

}  // namespace maze_gen
