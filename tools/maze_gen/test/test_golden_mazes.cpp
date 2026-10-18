// Mazes saved in test/golden/, one per algorithm, all from the same size and seed.
// Each test generates the maze again and compares it with the saved one, so they fail
// if a change alters what a seed produces, on any compiler or platform.
//
// When an algorithm is changed on purpose, regenerate the files with
//   MAZE_GEN_UPDATE_GOLDEN=1 ./test_golden_mazes
// and review the diff: every seed that users have stored will give a different maze.
#include <gtest/gtest.h>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include "maze_gen/maze_generator.hpp"
#include "maze_gen/maze_grid.hpp"
#include "maze_test_utils.hpp"

using namespace maze_gen;  // NOLINT(build/namespaces)

namespace
{

constexpr int kWidth = 15;
constexpr int kHeight = 10;
constexpr uint64_t kSeed = 42;

std::filesystem::path golden_path(MazeAlgorithm algorithm)
{
  return std::filesystem::path(MAZE_GEN_GOLDEN_DIR) /
         (std::string(to_string(algorithm)) + "_seed42.maze");
}

}  // namespace

class GoldenMaze : public ::testing::TestWithParam<MazeAlgorithm> {};

TEST_P(GoldenMaze, SameSeedGivesTheSavedMaze) {
  MazeStrategyOptions options;
  options.seed = kSeed;
  const MazeGrid generated = MazeGenerator(GetParam()).generate(kWidth, kHeight, options);
  const auto path = golden_path(GetParam());

  if (std::getenv("MAZE_GEN_UPDATE_GOLDEN") != nullptr) {
    std::ofstream(path) << maze_gen::test::serialize(generated);
  }

  const MazeGrid saved(path);
  ASSERT_EQ(saved.width(), kWidth);
  ASSERT_EQ(saved.height(), kHeight);
  for (int y = 0; y < kHeight; ++y) {
    for (int x = 0; x < kWidth; ++x) {
      ASSERT_EQ(generated.walls(x, y), saved.walls(x, y)) << "cell " << x << "," << y;
    }
  }
  EXPECT_EQ(generated.exits(), saved.exits());
}

INSTANTIATE_TEST_SUITE_P(
  AllAlgorithms, GoldenMaze,
  ::testing::ValuesIn(all_algorithms().begin(), all_algorithms().end()),
  [](const ::testing::TestParamInfo<MazeAlgorithm> & info) {
    return std::string(to_string(info.param));
  });
