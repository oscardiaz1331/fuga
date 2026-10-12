#include "maze_gen/maze_generator.hpp"
#include "maze_gen/maze_strategy_register.hpp"

namespace maze_gen {

MazeGenerator::MazeGenerator(MazeAlgorithm strategy):
	_strategy(strategy), _roomGenerator(), _exitGenerator(), _resolver()
{
	MazeStrategyRegister::registerAllStrategies();
	setMazeStrategy(strategy);
}

void MazeGenerator::setMazeStrategy(MazeAlgorithm algo)
{
	_strategy = algo;
	_mazeStrategy = MazeStrategyFactory::instance().create(algo);
}

void MazeGenerator::reset(MazeGrid& grid)
{
	auto cell_type = _mapper.getCellType(_strategy);
	grid.reset(cell_type);
}

void MazeGenerator::generate(MazeGrid& grid,const MazeStrategyOptions& options)
{
	reset(grid);
	_mazeStrategy->setDifficulty(options.difficulty);
	_mazeStrategy->setSeed(options.seed);
	if (_strategy != MazeAlgorithm::CellularAutomaton) {
		_roomGenerator.generateRooms(grid, options);
	}
	_mazeStrategy->generateMaze(grid);
	_exitGenerator.generateExits(grid, options);
	if(!_resolver.areAllExitsConnected(grid)) {
		_resolver.connectAllExits(grid);
	}
}

}  // namespace maze_gen
