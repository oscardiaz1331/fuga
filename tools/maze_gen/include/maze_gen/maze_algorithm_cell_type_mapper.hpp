#pragma once
#include "maze_gen/cell.hpp"
#include "maze_gen/maze_algorithm.hpp"
#include <unordered_map>

namespace maze_gen {

class MazeAlgorithmCellTypeMapper {
public:
    static CellType getCellType(MazeAlgorithm algorithm) {
        static const std::unordered_map<MazeAlgorithm, CellType> algorithmToCellType = {
            {MazeAlgorithm::DFS, CellType::VisitedCell},
            {MazeAlgorithm::AldousBroder, CellType::VisitedCell},
            {MazeAlgorithm::CellularAutomaton, CellType::AliveCell},
            {MazeAlgorithm::Ellers, CellType::Cell},
            {MazeAlgorithm::BinaryTree, CellType::Cell},
            {MazeAlgorithm::GrowingTree, CellType::VisitedCell},
            {MazeAlgorithm::HuntAndKill, CellType::VisitedCell},
            {MazeAlgorithm::Kruskal, CellType::Cell},
            {MazeAlgorithm::Prim, CellType::Cell},
            {MazeAlgorithm::RecursiveDivision, CellType::Cell},
            {MazeAlgorithm::Sidewinder, CellType::Cell},
            {MazeAlgorithm::Wilson, CellType::Cell}
        };

        auto it = algorithmToCellType.find(algorithm);
        return (it != algorithmToCellType.end()) ? it->second : CellType::Cell;
    }
};

}  // namespace maze_gen
