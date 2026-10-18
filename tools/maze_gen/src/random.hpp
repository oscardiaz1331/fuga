#pragma once
#include <cstddef>
#include <cstdint>
#include <optional>
#include <random>
#include <utility>

namespace maze_gen
{

// Random numbers that come out the same on every compiler for a given seed.
// std::mt19937 is specified bit by bit, but std::shuffle and the std distributions
// are not, so this class does that part itself.
class Random
{
public:
  // The stream separates the phases of one generation (algorithm, rooms, exits...):
  // they share the seed but must not replay the same sequence
  explicit Random(std::optional<uint64_t> seed, uint32_t stream = 0);

  uint32_t next() {return static_cast<uint32_t>(engine_());}
  // Uniform in [0, n). n has to be greater than 0
  uint32_t below(uint32_t n);
  // Uniform in [lo, hi], both included
  int uniform_int(int lo, int hi);
  // Uniform position in a container of n elements
  size_t index(size_t n) {return below(static_cast<uint32_t>(n));}
  // Uniform in [0, 1)
  double unit() {return next() * (1.0 / 4294967296.0);}
  // True with probability p: never for 0, always for 1
  bool chance(double p) {return unit() < p;}

  template<class RandomIt>
  void shuffle(RandomIt first, RandomIt last)
  {
    // Fisher-Yates
    for (auto n = last - first; n > 1; --n) {
      std::swap(first[n - 1], first[below(static_cast<uint32_t>(n))]);
    }
  }

private:
  std::mt19937 engine_;
};

}  // namespace maze_gen
