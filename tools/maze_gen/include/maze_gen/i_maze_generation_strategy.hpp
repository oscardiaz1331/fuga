#pragma once
#include "maze_gen/maze_grid.hpp"
#include <random>
#include "maze_gen/random_generator.hpp"

namespace maze_gen {

class IMazeGenerationStrategy
{
public:
	virtual void generateMaze(MazeGrid& grid) = 0;
	virtual ~IMazeGenerationStrategy() = default;
	virtual void setSeed(std::optional<uint64_t> seed) { _seed = seed; } // nullopt = seed from random_device
	virtual void setDifficulty(int difficulty) {} // Optional, not all strategies use it

protected:
    std::optional<uint64_t> _seed; // accessible to derived classes

    // Helper: construct a seeded mt19937 deterministically using the 64-bit seed.
    // If no seed is set, it seeds from random_device.
    std::mt19937 make_rng() const {
        return RandomGenerator::generateRng(_seed);
    }
};

}  // namespace maze_gen
