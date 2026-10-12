#pragma once
#include "maze_gen/i_maze_generation_strategy.hpp"
#include <random>

namespace maze_gen {

class CellularAutomatonStrategy :
    public IMazeGenerationStrategy
{
public:
	void generateMaze(MazeGrid& grid) override;
	void setDifficulty(int difficulty) override;
private:
	void initializeGrid(MazeGrid& grid);
	void applyRules(MazeGrid& grid);
	void ensureConnectivity(MazeGrid& grid);
	void connectPoints(MazeGrid& grid, int x1, int y1, int x2, int y2);
	void terminate(MazeGrid& grid);
	std::mt19937 _rng{make_rng()};
	float _initialThreshold = 0.35f; // Initial threshold for cell activation
	int _iterations = 5; // Number of iterations for the cellular automaton
	float _birthThreshold = 0.45f; // Threshold for cell birth
	float _deathThreshold = 0.55f; // Threshold for cell death
	int _num_neighbours = 4; // Difficulty level, can be set externally
};

}  // namespace maze_gen
