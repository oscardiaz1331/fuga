// Every generation algorithm on its own: no braiding, rooms, exits or connector.
#include <gtest/gtest.h>
#include <string>
#include <tuple>
#include <vector>
#include "maze_test_utils.hpp"

using namespace maze_gen;  // NOLINT(build/namespaces)
using maze_gen::test::analyze;
using maze_gen::test::is_perfect;
using maze_gen::test::run_strategy;
using maze_gen::test::walls_of;

namespace
{

struct GridSize
{
  int width;
  int height;
};

// Degenerate shapes first: they are where loops and index arithmetic break
const GridSize kSizes[] = {
  {1, 1}, {1, 2}, {2, 1}, {1, 9}, {9, 1}, {2, 2}, {2, 7}, {7, 2},
  {3, 3}, {5, 4}, {10, 10}, {17, 31}, {40, 25}
};

const uint64_t kSeeds[] = {1, 2, 3, 42, 123456789, 0xFFFFFFFFFFFFFFFFull};

std::string algorithm_param_name(const ::testing::TestParamInfo<MazeAlgorithm> & info)
{
  return test::algorithm_name(info.param);
}

int dead_ends_over_five_seeds(MazeAlgorithm algorithm, const MazeStrategyOptions & options = {})
{
  int total = 0;
  for (uint64_t seed = 1; seed <= 5; ++seed) {
    total += analyze(run_strategy(algorithm, 30, 30, seed, options)).dead_ends;
  }
  return total;
}

}  // namespace

// ===================== perfect mazes =====================

class PerfectMazeTest : public ::testing::TestWithParam<std::tuple<MazeAlgorithm, GridSize>>
{
protected:
  MazeAlgorithm algorithm() const {return std::get<0>(GetParam());}
  int width() const {return std::get<1>(GetParam()).width;}
  int height() const {return std::get<1>(GetParam()).height;}
};

TEST_P(PerfectMazeTest, IsConnectedAndHasNoLoops) {
  for (uint64_t seed : kSeeds) {
    const auto stats = analyze(run_strategy(algorithm(), width(), height(), seed));
    EXPECT_TRUE(is_perfect(stats))
      << "seed " << seed << ": regions=" << stats.regions
      << " closed_cells=" << stats.closed_cells
      << " passages=" << stats.passages << " (expected " << stats.cells - 1 << ")";
  }
}

TEST_P(PerfectMazeTest, LeavesTheOuterWallClosed) {
  for (uint64_t seed : kSeeds) {
    const auto stats = analyze(run_strategy(algorithm(), width(), height(), seed));
    EXPECT_EQ(stats.border_openings, 0) << "seed " << seed;
  }
}

TEST_P(PerfectMazeTest, WallsAgreeOnBothSides) {
  for (uint64_t seed : kSeeds) {
    const auto stats = analyze(run_strategy(algorithm(), width(), height(), seed));
    EXPECT_EQ(stats.asymmetric_walls, 0) << "seed " << seed;
  }
}

TEST_P(PerfectMazeTest, WorksWithoutASeed) {
  const auto stats = analyze(run_strategy(algorithm(), width(), height(), std::nullopt));
  EXPECT_TRUE(is_perfect(stats));
  EXPECT_EQ(stats.border_openings, 0);
}

INSTANTIATE_TEST_SUITE_P(
  AllAlgorithms, PerfectMazeTest,
  ::testing::Combine(
    ::testing::ValuesIn(test::perfect_algorithms()), ::testing::ValuesIn(kSizes)),
  [](const ::testing::TestParamInfo<PerfectMazeTest::ParamType> & info) {
    const GridSize & size = std::get<1>(info.param);
    return test::algorithm_name(std::get<0>(info.param)) + "_" +
           test::size_name(size.width, size.height);
  });

// ===================== seeds =====================

class SeedTest : public ::testing::TestWithParam<MazeAlgorithm> {};

TEST_P(SeedTest, SameSeedGivesTheSameMaze) {
  EXPECT_EQ(
    walls_of(run_strategy(GetParam(), 20, 15, 42)),
    walls_of(run_strategy(GetParam(), 20, 15, 42)));
}

TEST_P(SeedTest, DifferentSeedsGiveDifferentMazes) {
  EXPECT_NE(
    walls_of(run_strategy(GetParam(), 20, 15, 1)),
    walls_of(run_strategy(GetParam(), 20, 15, 2)));
}

TEST_P(SeedTest, WithoutASeedEveryMazeIsDifferent) {
  EXPECT_NE(
    walls_of(run_strategy(GetParam(), 20, 15, std::nullopt)),
    walls_of(run_strategy(GetParam(), 20, 15, std::nullopt)));
}

// MazeGenerator keeps one strategy object and calls it again and again: a strategy
// must not carry anything over from one run to the next
TEST_P(SeedTest, ReusingTheStrategyObjectChangesNothing) {
  const MazeStrategyOptions options;
  auto strategy = algorithm_info(GetParam()).create();

  MazeGrid first(20, 15);
  Random first_rng(99);
  strategy->generate(first, first_rng, options);

  MazeGrid other(20, 15);
  Random other_rng(7);
  strategy->generate(other, other_rng, options);

  MazeGrid again(20, 15);
  Random again_rng(99);
  strategy->generate(again, again_rng, options);

  EXPECT_EQ(walls_of(first), walls_of(again));
  EXPECT_NE(walls_of(first), walls_of(other));
}

INSTANTIATE_TEST_SUITE_P(
  AllAlgorithms, SeedTest, ::testing::ValuesIn(test::every_algorithm()), algorithm_param_name);

// ===================== what makes each algorithm recognisable =====================

TEST(BinaryTreeShape, TopRowAndLeftColumnAreCorridors) {
  const MazeGrid grid = run_strategy(MazeAlgorithm::BinaryTree, 12, 9, 5);
  for (int x = 0; x + 1 < grid.width(); ++x) {
    EXPECT_FALSE(grid.has_wall(x, 0, Dir::E)) << "top row, x=" << x;
  }
  for (int y = 0; y + 1 < grid.height(); ++y) {
    EXPECT_FALSE(grid.has_wall(0, y, Dir::S)) << "left column, y=" << y;
  }
}

TEST(BinaryTreeShape, EveryOtherCellOpensExactlyNorthOrWest) {
  const MazeGrid grid = run_strategy(MazeAlgorithm::BinaryTree, 12, 9, 5);
  for (int y = 1; y < grid.height(); ++y) {
    for (int x = 1; x < grid.width(); ++x) {
      const bool north = !grid.has_wall(x, y, Dir::N);
      const bool west = !grid.has_wall(x, y, Dir::W);
      EXPECT_NE(north, west) << "cell " << x << "," << y;
    }
  }
}

TEST(SidewinderShape, TopRowIsACorridor) {
  const MazeGrid grid = run_strategy(MazeAlgorithm::Sidewinder, 12, 9, 5);
  for (int x = 0; x + 1 < grid.width(); ++x) {
    EXPECT_FALSE(grid.has_wall(x, 0, Dir::E)) << "x=" << x;
  }
}

TEST(SidewinderShape, EveryRunHasExactlyOneWayNorth) {
  const MazeGrid grid = run_strategy(MazeAlgorithm::Sidewinder, 12, 9, 5);
  for (int y = 1; y < grid.height(); ++y) {
    int north_openings = 0;
    for (int x = 0; x < grid.width(); ++x) {
      if (!grid.has_wall(x, y, Dir::N)) {
        ++north_openings;
      }
      const bool run_ends = x == grid.width() - 1 || grid.has_wall(x, y, Dir::E);
      if (run_ends) {
        EXPECT_EQ(north_openings, 1) << "run ending at " << x << "," << y;
        north_openings = 0;
      }
    }
  }
}

// DFS and Hunt-and-Kill carve long corridors; Prim and Kruskal branch all the time
TEST(MazeTexture, CorridorAlgorithmsHaveFarFewerDeadEnds) {
  const int dfs = dead_ends_over_five_seeds(MazeAlgorithm::DFS);
  const int hunt_and_kill = dead_ends_over_five_seeds(MazeAlgorithm::HuntAndKill);
  const int prim = dead_ends_over_five_seeds(MazeAlgorithm::Prim);
  const int kruskal = dead_ends_over_five_seeds(MazeAlgorithm::Kruskal);
  EXPECT_LT(dfs * 2, prim);
  EXPECT_LT(dfs * 2, kruskal);
  EXPECT_LT(hunt_and_kill * 2, prim);
  EXPECT_LT(hunt_and_kill * 2, kruskal);
}

// Aldous-Broder, Wilson and Houston all aim at mazes with no bias, so on average
// they have to look alike
TEST(MazeTexture, UnbiasedAlgorithmsAgreeOnTheShareOfDeadEnds) {
  auto dead_ends = [](MazeAlgorithm algorithm) {
      int total = 0;
      for (uint64_t seed = 1; seed <= 20; ++seed) {
        total += analyze(run_strategy(algorithm, 30, 30, seed)).dead_ends;
      }
      return total / 20.0;
    };
  const double aldous_broder = dead_ends(MazeAlgorithm::AldousBroder);
  EXPECT_NEAR(dead_ends(MazeAlgorithm::Wilson), aldous_broder, 0.05 * aldous_broder);
  EXPECT_NEAR(dead_ends(MazeAlgorithm::Houston), aldous_broder, 0.05 * aldous_broder);
}

// ===================== parameters of individual algorithms =====================

TEST(GrowingTreeOption, AlwaysNewestBehavesLikeDFS) {
  MazeStrategyOptions newest;
  newest.growing_tree_newest_ratio = 1.0;
  const int growing_tree = dead_ends_over_five_seeds(MazeAlgorithm::GrowingTree, newest);
  const int dfs = dead_ends_over_five_seeds(MazeAlgorithm::DFS);
  EXPECT_NEAR(growing_tree, dfs, 0.15 * dfs);
}

TEST(GrowingTreeOption, AlwaysRandomBehavesLikePrim) {
  MazeStrategyOptions random;
  random.growing_tree_newest_ratio = 0.0;
  const int growing_tree = dead_ends_over_five_seeds(MazeAlgorithm::GrowingTree, random);
  const int prim = dead_ends_over_five_seeds(MazeAlgorithm::Prim);
  const int dfs = dead_ends_over_five_seeds(MazeAlgorithm::DFS);
  EXPECT_NEAR(growing_tree, prim, 0.25 * prim);
  EXPECT_GT(growing_tree, 2 * dfs);
}

// The share of dead ends does not fall steadily with the ratio (it peaks around one
// half), but near 1 every step towards "newest" makes the corridors longer
TEST(GrowingTreeOption, TheLastStretchTowardsNewestRemovesDeadEnds) {
  int previous = 1 << 30;
  for (double ratio : {0.75, 0.9, 1.0}) {
    MazeStrategyOptions options;
    options.growing_tree_newest_ratio = ratio;
    const int dead_ends = dead_ends_over_five_seeds(MazeAlgorithm::GrowingTree, options);
    EXPECT_LT(dead_ends, previous) << "ratio " << ratio;
    previous = dead_ends;
  }
}

TEST(GrowingTreeOption, StaysPerfectForAnyRatio) {
  for (double ratio : {0.0, 0.25, 0.5, 0.75, 1.0}) {
    MazeStrategyOptions options;
    options.growing_tree_newest_ratio = ratio;
    EXPECT_TRUE(is_perfect(analyze(run_strategy(MazeAlgorithm::GrowingTree, 20, 20, 3, options))))
      << "ratio " << ratio;
  }
}

TEST(SidewinderOption, ZeroMakesEveryCellItsOwnRun) {
  MazeStrategyOptions options;
  options.sidewinder_east_probability = 0.0;
  const MazeGrid grid = run_strategy(MazeAlgorithm::Sidewinder, 10, 8, 3, options);
  EXPECT_TRUE(is_perfect(analyze(grid)));
  for (int y = 1; y < grid.height(); ++y) {
    for (int x = 0; x < grid.width(); ++x) {
      EXPECT_FALSE(grid.has_wall(x, y, Dir::N)) << "cell " << x << "," << y;
      if (x + 1 < grid.width()) {
        EXPECT_TRUE(grid.has_wall(x, y, Dir::E)) << "cell " << x << "," << y;
      }
    }
  }
}

TEST(SidewinderOption, OneMakesEveryRowASingleRun) {
  MazeStrategyOptions options;
  options.sidewinder_east_probability = 1.0;
  const MazeGrid grid = run_strategy(MazeAlgorithm::Sidewinder, 10, 8, 3, options);
  EXPECT_TRUE(is_perfect(analyze(grid)));
  for (int y = 0; y < grid.height(); ++y) {
    int north_openings = 0;
    for (int x = 0; x < grid.width(); ++x) {
      if (x + 1 < grid.width()) {
        EXPECT_FALSE(grid.has_wall(x, y, Dir::E)) << "cell " << x << "," << y;
      }
      if (y > 0 && !grid.has_wall(x, y, Dir::N)) {
        ++north_openings;
      }
    }
    if (y > 0) {
      EXPECT_EQ(north_openings, 1) << "row " << y;
    }
  }
}

TEST(EllersOption, StaysPerfectAtBothExtremes) {
  for (double probability : {0.0, 0.3, 1.0}) {
    MazeStrategyOptions options;
    options.ellers_join_probability = probability;
    for (uint64_t seed = 1; seed <= 5; ++seed) {
      EXPECT_TRUE(is_perfect(analyze(run_strategy(MazeAlgorithm::Ellers, 15, 12, seed, options))))
        << "probability " << probability << ", seed " << seed;
    }
  }
}

TEST(EllersOption, ZeroLeavesAllTheJoiningToTheLastRow) {
  MazeStrategyOptions options;
  options.ellers_join_probability = 0.0;
  const MazeGrid grid = run_strategy(MazeAlgorithm::Ellers, 10, 6, 3, options);
  for (int y = 0; y + 1 < grid.height(); ++y) {
    for (int x = 0; x + 1 < grid.width(); ++x) {
      EXPECT_TRUE(grid.has_wall(x, y, Dir::E)) << "cell " << x << "," << y;
    }
  }
  for (int x = 0; x + 1 < grid.width(); ++x) {
    EXPECT_FALSE(grid.has_wall(x, grid.height() - 1, Dir::E)) << "last row, x=" << x;
  }
}

// The options of one algorithm must not leak into the others
TEST(AlgorithmOptions, OnlyAffectTheirOwnAlgorithm) {
  MazeStrategyOptions changed;
  changed.growing_tree_newest_ratio = 1.0;
  changed.sidewinder_east_probability = 0.9;
  changed.ellers_join_probability = 0.1;
  changed.cave_level = 7;
  changed.difficulty = 0;
  changed.room_level = 10;
  for (MazeAlgorithm algorithm : {MazeAlgorithm::DFS, MazeAlgorithm::Kruskal,
      MazeAlgorithm::Prim, MazeAlgorithm::Wilson, MazeAlgorithm::BinaryTree})
  {
    EXPECT_EQ(
      walls_of(run_strategy(algorithm, 12, 12, 5, changed)),
      walls_of(run_strategy(algorithm, 12, 12, 5))) << test::algorithm_name(algorithm);
  }
}

// ===================== regressions =====================

// Wilson used to keep a stale step when erasing a loop and carved extra walls
TEST(Regression, WilsonCarvesNoExtraPassages) {
  for (uint64_t seed = 1; seed <= 20; ++seed) {
    const auto stats = analyze(run_strategy(MazeAlgorithm::Wilson, 30, 30, seed));
    EXPECT_EQ(stats.passages, 899) << "seed " << seed;
  }
}

// The hunt phase used to start a new, unconnected piece every time
TEST(Regression, HuntAndKillStaysInOnePiece) {
  for (uint64_t seed = 1; seed <= 20; ++seed) {
    const auto stats = analyze(run_strategy(MazeAlgorithm::HuntAndKill, 30, 30, seed));
    EXPECT_EQ(stats.regions, 1) << "seed " << seed;
    EXPECT_EQ(stats.closed_cells, 0) << "seed " << seed;
  }
}

// It used to carve where it had to build a wall, leaving bands of sealed cells
TEST(Regression, RecursiveDivisionBuildsAMaze) {
  for (uint64_t seed = 1; seed <= 20; ++seed) {
    EXPECT_TRUE(is_perfect(analyze(run_strategy(MazeAlgorithm::RecursiveDivision, 30, 30, seed))))
      << "seed " << seed;
  }
}

// It used to carve north and west on the border cells too
TEST(Regression, BinaryTreeDoesNotOpenTheBorder) {
  for (uint64_t seed = 1; seed <= 20; ++seed) {
    const auto stats = analyze(run_strategy(MazeAlgorithm::BinaryTree, 30, 30, seed));
    EXPECT_EQ(stats.border_openings, 0) << "seed " << seed;
    EXPECT_TRUE(is_perfect(stats)) << "seed " << seed;
  }
}

// With a single cell the active list was never emptied
TEST(Regression, GrowingTreeFinishesOnASingleCell) {
  EXPECT_EQ(analyze(run_strategy(MazeAlgorithm::GrowingTree, 1, 1, 1)).passages, 0);
}

// ===================== cave generator =====================

class CaveTest : public ::testing::TestWithParam<int>
{
protected:
  MazeGrid cave(int width, int height, uint64_t seed) const
  {
    MazeStrategyOptions options;
    options.cave_level = GetParam();
    return run_strategy(MazeAlgorithm::CellularAutomaton, width, height, seed, options);
  }
};

TEST_P(CaveTest, IsASingleRegion) {
  for (uint64_t seed = 1; seed <= 15; ++seed) {
    EXPECT_LE(analyze(cave(30, 30, seed)).regions, 1) << "seed " << seed;
  }
}

TEST_P(CaveTest, LeavesTheOuterWallClosedAndWallsConsistent) {
  for (uint64_t seed = 1; seed <= 15; ++seed) {
    const auto stats = analyze(cave(30, 30, seed));
    EXPECT_EQ(stats.border_openings, 0) << "seed " << seed;
    EXPECT_EQ(stats.asymmetric_walls, 0) << "seed " << seed;
  }
}

TEST_P(CaveTest, HasBothOpenSpaceAndRock) {
  const auto stats = analyze(cave(40, 40, 11));
  EXPECT_GT(stats.passages, 0);
  EXPECT_GT(stats.closed_cells, 0);
  EXPECT_LT(stats.closed_cells, stats.cells);
}

TEST_P(CaveTest, HandlesTinyGrids) {
  for (const GridSize & size : kSizes) {
    if (size.width * size.height > 25) {
      continue;
    }
    const auto stats = analyze(cave(size.width, size.height, 3));
    EXPECT_LE(stats.regions, 1) << test::size_name(size.width, size.height);
    EXPECT_EQ(stats.border_openings, 0) << test::size_name(size.width, size.height);
  }
}

INSTANTIATE_TEST_SUITE_P(EveryLevel, CaveTest, ::testing::Range(0, 11));

// A level that jumped up and down would be useless as a control
TEST(CaveLevel, MoreLevelMeansMoreRock) {
  auto rock = [](int level) {
      MazeStrategyOptions options;
      options.cave_level = level;
      int total = 0;
      for (uint64_t seed = 1; seed <= 20; ++seed) {
        total += analyze(
          run_strategy(MazeAlgorithm::CellularAutomaton, 30, 30, seed, options)).closed_cells;
      }
      return total;
    };
  int previous = -1;
  for (int level = 0; level <= 10; ++level) {
    const int current = rock(level);
    EXPECT_GT(current, previous) << "level " << level;
    previous = current;
  }
}
