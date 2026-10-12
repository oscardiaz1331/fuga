// StrategyFactory.hpp
#pragma once
#include <functional>
#include <memory>
#include <unordered_map>
#include <string>
#include "maze_gen/i_maze_generation_strategy.hpp"
#include "maze_gen/maze_algorithm.hpp"
#include "maze_gen/maze_strategy_options.hpp"

namespace maze_gen {

class MazeStrategyFactory {
public:
    using Creator = std::function<std::unique_ptr<IMazeGenerationStrategy>()>;

    static MazeStrategyFactory& instance(); // Meyers singleton

    void registerCreator(MazeAlgorithm key, Creator c);
    std::unique_ptr<IMazeGenerationStrategy> create(MazeAlgorithm key) const;

private:
    MazeStrategyFactory() = default;
    std::unordered_map<MazeAlgorithm, Creator> _map;
};

}  // namespace maze_gen
