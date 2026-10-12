#include "maze_gen/random_generator.hpp"

namespace maze_gen {

std::mt19937 RandomGenerator::generateRng(std::optional<uint64_t> seed)
{
    if (seed) {
        uint64_t s = *seed;
        // split 64-bit seed into two 32-bit words to feed seed_seq
        std::seed_seq seq{
            static_cast<uint32_t>(s & 0xffffffffu),
            static_cast<uint32_t>(s >> 32)
        };
        return std::mt19937(seq);
    }
    else {
        // seed with random_device
        return std::mt19937(static_cast<uint32_t>(std::random_device{}()));
    }
}

}  // namespace maze_gen
