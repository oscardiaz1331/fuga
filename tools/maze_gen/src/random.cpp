#include "random.hpp"
#include <cassert>

namespace maze_gen
{

namespace
{

std::mt19937 make_engine(std::optional<uint64_t> seed, uint32_t stream)
{
  if (seed) {
    // std::seed_seq is fully specified, unlike seeding from a single integer of unknown width
    std::seed_seq sequence{
      static_cast<uint32_t>(*seed & 0xffffffffu),
      static_cast<uint32_t>(*seed >> 32),
      stream
    };
    return std::mt19937(sequence);
  }
  std::random_device device;
  std::seed_seq sequence{device(), device(), device(), device()};
  return std::mt19937(sequence);
}

}  // namespace

Random::Random(std::optional<uint64_t> seed, uint32_t stream)
: engine_(make_engine(seed, stream))
{
}

uint32_t Random::below(uint32_t n)
{
  assert(n > 0);
  // Lemire's method: scale the 32 random bits to [0, n) with one multiplication and
  // take the high half. 2^32 is rarely a multiple of n, so a few low halves have to be
  // rejected to leave every result equally likely; the division needed to know which
  // ones is only computed when the low half is small enough to be one of them
  uint64_t product = static_cast<uint64_t>(next()) * n;
  uint32_t low = static_cast<uint32_t>(product);
  if (low < n) {
    const uint32_t rejected = (0u - n) % n;
    while (low < rejected) {
      product = static_cast<uint64_t>(next()) * n;
      low = static_cast<uint32_t>(product);
    }
  }
  return static_cast<uint32_t>(product >> 32);
}

int Random::uniform_int(int lo, int hi)
{
  assert(lo <= hi);
  const uint32_t span = static_cast<uint32_t>(hi) - static_cast<uint32_t>(lo);
  return lo + static_cast<int>(below(span + 1));
}

}  // namespace maze_gen
