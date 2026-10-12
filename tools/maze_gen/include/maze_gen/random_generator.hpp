#pragma once
#include <random>
#include <optional>

namespace maze_gen {

class RandomGenerator
{
public:
	static std::mt19937 generateRng(std::optional<uint64_t> seed);
};

}  // namespace maze_gen
