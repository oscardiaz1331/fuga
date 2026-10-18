// A seed has to give the same maze on every compiler and platform. These tests pin
// down exact values, so they fail if a change alters what a seed produces, and they
// are the check to run when building somewhere new.
//
// When the output of an algorithm is changed on purpose, the values below have to be
// updated, and every seed that users have stored will give a different maze.
#include <gtest/gtest.h>
#include <string>
#include <vector>
#include "maze_gen/maze_generator.hpp"
#include "maze_test_utils.hpp"

using namespace maze_gen;  // NOLINT(build/namespaces)

// ---------- random numbers ----------

TEST(ReproducibleRandom, RawSequence) {
  Random rng(42);
  const std::vector<uint32_t> expected = {
    4072340654u, 4282665192u, 3415449314u, 1961144635u, 2139743462u};
  for (uint32_t value : expected) {
    EXPECT_EQ(rng.next(), value);
  }
}

TEST(ReproducibleRandom, RawSequenceOfAnotherStream) {
  Random rng(42, 3);
  for (uint32_t value : {3150408723u, 1156686416u, 3091651171u}) {
    EXPECT_EQ(rng.next(), value);
  }
}

TEST(ReproducibleRandom, RawSequenceOfTheLargestSeed) {
  Random rng(0xFFFFFFFFFFFFFFFFull);
  for (uint32_t value : {826912331u, 3146607743u, 1072119660u}) {
    EXPECT_EQ(rng.next(), value);
  }
}

TEST(ReproducibleRandom, UniformInt) {
  Random rng(42);
  for (int value : {9, 9, 7, 4, 4, 7, 8, 9, 3, 5, 9, 2}) {
    EXPECT_EQ(rng.uniform_int(0, 9), value);
  }
}

TEST(ReproducibleRandom, Below) {
  Random rng(42);
  for (uint32_t value : {2u, 2u, 2u, 1u, 1u, 2u, 2u, 2u, 1u, 1u, 2u, 0u}) {
    EXPECT_EQ(rng.below(3), value);
  }
}

TEST(ReproducibleRandom, Shuffle) {
  Random rng(42);
  std::vector<int> values = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  rng.shuffle(values.begin(), values.end());
  EXPECT_EQ(values, (std::vector<int>{1, 0, 5, 4, 7, 2, 3, 6, 8, 9}));
}

TEST(ReproducibleRandom, Unit) {
  Random rng(42);
  // 32 random bits divided by 2^32: exact in a double, so equality is the right check
  EXPECT_EQ(rng.unit(), 4072340654.0 / 4294967296.0);
  EXPECT_EQ(rng.unit(), 4282665192.0 / 4294967296.0);
}

// ---------- every algorithm ----------

namespace
{

struct GoldenMaze
{
  MazeAlgorithm algorithm;
  // The wall mask of each cell of an 8x5 maze made with seed 42, one hexadecimal digit
  // per cell, row by row
  const char * walls;
};

const GoldenMaze kGoldenMazes[] = {
  {MazeAlgorithm::DFS, "b955155386b92d56c3aae953baac5696c44557c7"},
  {MazeAlgorithm::AldousBroder, "97953d53c3c78796ba95296bc2ed4052d45556d6"},
  {MazeAlgorithm::CellularAutomaton, "d1155513fc6fff82ffffff869511112fefc4446f"},
  {MazeAlgorithm::Ellers, "bbbd17bbc04507c2beb92bbaa906ea82c6c5546e"},
  {MazeAlgorithm::BinaryTree, "9511115787eea8138557eaee81513853ec7eec7e"},
  {MazeAlgorithm::GrowingTree, "bd3d3bbbc545042ad5396d42952e9792ed45456e"},
  {MazeAlgorithm::HuntAndKill, "955553d385113c56c3aae953baac5696c6c557c7"},
  {MazeAlgorithm::Kruskal, "b9517b97ae96d047c3851417d047c3c3d457d6d6"},
  {MazeAlgorithm::Prim, "d3d53d53bad52952c4552e9691150107eeed6ec7"},
  {MazeAlgorithm::RecursiveDivision, "d17bb93bb8502aea807a843aec7ead2ad5556d46"},
  {MazeAlgorithm::Sidewinder, "91511117aed2eec383d4153aa87943aaec7c7eee"},
  {MazeAlgorithm::Wilson, "b9155557c2abd17bd6ac5052d50516d6d56d4557"},
  {MazeAlgorithm::Houston, "bbbb97d3846c2b968555404783d55053ec7d56d6"},
};

}  // namespace

TEST(ReproducibleMazes, EveryAlgorithmHasAPinnedMaze) {
  EXPECT_EQ(std::size(kGoldenMazes), all_algorithms().size())
    << "add a line to kGoldenMazes for the new algorithm";
}

TEST(ReproducibleMazes, EachAlgorithmGivesItsPinnedMaze) {
  for (const GoldenMaze & golden : kGoldenMazes) {
    EXPECT_EQ(test::walls_text(test::run_strategy(golden.algorithm, 8, 5, 42)), golden.walls)
      << to_string(golden.algorithm);
  }
}

// ---------- the whole pipeline ----------

// Algorithm, dead ends, rooms and exits, each with its own random sequence
TEST(ReproducibleMazes, FullGeneration) {
  MazeStrategyOptions options;
  options.seed = 2024;
  options.num_random_exits = 3;
  options.difficulty = 6;
  options.room_level = 4;
  const MazeGrid grid = MazeGenerator{MazeAlgorithm::Kruskal}.generate(12, 9, options);

  EXPECT_EQ(
    test::walls_text(grid),
    "915513914397ac53aaac7a870116ac015683aead416c1106a9053a916ec386852a801796ad296c468547"
    "a92879152913ec6856c7c6c6");
  const std::vector<Exit> expected_exits = {{3, 8, Dir::S}, {8, 0, Dir::N}, {0, 2, Dir::W}};
  EXPECT_EQ(grid.exits(), expected_exits);
}
