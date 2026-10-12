#include "maze_gen/maze_strategy_factory.hpp"
#include <stdexcept>

namespace maze_gen {

MazeStrategyFactory& MazeStrategyFactory::instance()
{
    static MazeStrategyFactory f;
    return f;
}

void MazeStrategyFactory::registerCreator(MazeAlgorithm key, Creator c)
{
    _map.try_emplace(key, std::move(c));
}

std::unique_ptr<IMazeGenerationStrategy> MazeStrategyFactory::create(MazeAlgorithm key) const
{
    auto it = _map.find(key);
    if (it == _map.end()) throw std::runtime_error("Unknown algorithm enum");
    return it->second();
}

}  // namespace maze_gen
