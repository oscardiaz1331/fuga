#pragma once
#include <memory>
#include "maze_gen/i_maze_generation_strategy.hpp"
#include "maze_gen/maze_algorithm.hpp"
#include "maze_gen/maze_strategy_options.hpp"
#include "maze_gen/maze_strategy_factory.hpp"
#include "maze_gen/maze_algorithm_cell_type_mapper.hpp"
#include "maze_gen/room_generator.hpp"
#include "maze_gen/maze_exit_generator.hpp"
#include "maze_gen/maze_resolver.hpp"

namespace maze_gen {

class MazeGenerator
{
private:
	std::unique_ptr<IMazeGenerationStrategy> _mazeStrategy;
	MazeAlgorithm _strategy;
	MazeAlgorithmCellTypeMapper _mapper{};
	RoomGenerator _roomGenerator;
	MazeExitGenerator _exitGenerator;
	MazeResolver _resolver;

	void reset(MazeGrid& grid);

public:
	MazeGenerator(MazeAlgorithm strategy = MazeAlgorithm::AldousBroder);

	void setMazeStrategy(MazeAlgorithm algo);

	void generate(MazeGrid& grid, const MazeStrategyOptions& options);

	virtual ~MazeGenerator() = default;
};

}  // namespace maze_gen
