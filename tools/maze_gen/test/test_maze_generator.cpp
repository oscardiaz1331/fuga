// MazeGenerator::generate from end to end: algorithm, dead ends, rooms, exits and
// connection, plus the validation of the options.
#include <gtest/gtest.h>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include "maze_gen/maze_generator.hpp"
#include "maze_test_utils.hpp"

using namespace maze_gen;  // NOLINT(build/namespaces)
using maze_gen::test::analyze;
using maze_gen::test::walls_of;

namespace
{

MazeStrategyOptions options(std::optional<uint64_t> seed, int exits = 2)
{
  MazeStrategyOptions opts;
  opts.seed = seed;
  opts.num_random_exits = exits;
  return opts;
}

MazeGrid generate(
  MazeAlgorithm algorithm, int width, int height, const MazeStrategyOptions & opts)
{
  return MazeGenerator{algorithm}.generate(width, height, opts);
}

// An exit is a border cell whose outer wall is open
void expect_valid_exit(const MazeGrid & grid, const Exit & exit)
{
  ASSERT_TRUE(grid.in_bounds(exit.x, exit.y)) << ::testing::PrintToString(exit);
  const auto [nx, ny] = MazeGrid::neighbor(exit.x, exit.y, exit.dir);
  EXPECT_FALSE(grid.in_bounds(nx, ny)) << ::testing::PrintToString(exit) << " points inside";
  EXPECT_FALSE(grid.has_wall(exit.x, exit.y, exit.dir))
    << ::testing::PrintToString(exit) << " is walled";
}

// The message of the exception thrown by generate(), or "" if it did not throw
std::string rejection(const MazeStrategyOptions & opts, int width = 10, int height = 10)
{
  MazeGrid grid(width, height);
  try {
    MazeGenerator{MazeAlgorithm::DFS}.generate(grid, opts);
  } catch (const std::invalid_argument & error) {
    return error.what();
  }
  return "";
}

std::string algorithm_param_name(const ::testing::TestParamInfo<MazeAlgorithm> & info)
{
  return test::algorithm_name(info.param);
}

}  // namespace

class GeneratorTest : public ::testing::TestWithParam<MazeAlgorithm> {};

// ---------- the result is usable ----------

TEST_P(GeneratorTest, EveryOpenCellIsReachable) {
  for (uint64_t seed = 1; seed <= 10; ++seed) {
    for (int exits : {0, 1, 4}) {
      const auto stats = analyze(generate(GetParam(), 30, 30, options(seed, exits)));
      EXPECT_EQ(stats.regions, 1) << "seed " << seed << ", " << exits << " exits";
      EXPECT_EQ(stats.asymmetric_walls, 0) << "seed " << seed << ", " << exits << " exits";
    }
  }
}

TEST_P(GeneratorTest, MakesExactlyTheRequestedExits) {
  for (int exits : {0, 1, 2, 7}) {
    const MazeGrid grid = generate(GetParam(), 20, 20, options(5, exits));
    EXPECT_EQ(grid.exits().size(), static_cast<size_t>(exits));
    EXPECT_EQ(analyze(grid).border_openings, exits);
    for (const Exit & exit : grid.exits()) {
      expect_valid_exit(grid, exit);
    }
  }
}

TEST_P(GeneratorTest, RandomExitsAreDistinctAndAvoidTheCorners) {
  const MazeGrid grid = generate(GetParam(), 15, 10, options(9, 12));
  const std::set<Exit> unique(grid.exits().begin(), grid.exits().end());
  EXPECT_EQ(unique.size(), 12u);
  for (const Exit & exit : grid.exits()) {
    const bool corner = (exit.x == 0 || exit.x == grid.width() - 1) &&
      (exit.y == 0 || exit.y == grid.height() - 1);
    EXPECT_FALSE(corner) << ::testing::PrintToString(exit);
  }
}

// An exit that opens onto solid rock or a sealed pocket would be useless
TEST_P(GeneratorTest, EveryExitLeadsIntoTheMaze) {
  for (uint64_t seed = 1; seed <= 10; ++seed) {
    MazeStrategyOptions opts = options(seed, 6);
    opts.cave_level = 9;
    const MazeGrid grid = generate(GetParam(), 25, 25, opts);
    for (const Exit & exit : grid.exits()) {
      bool has_passage = false;
      for (const auto & [nx, ny, d] : grid.neighbors(exit.x, exit.y)) {
        has_passage = has_passage || !grid.has_wall(exit.x, exit.y, d);
      }
      EXPECT_TRUE(has_passage) << "seed " << seed << ", " << ::testing::PrintToString(exit);
    }
  }
}

// ---------- seeds ----------

TEST_P(GeneratorTest, SameSeedGivesTheSameMazeAndExits) {
  const MazeGrid first = generate(GetParam(), 25, 18, options(77, 3));
  const MazeGrid second = generate(GetParam(), 25, 18, options(77, 3));
  EXPECT_EQ(walls_of(first), walls_of(second));
  EXPECT_EQ(first.exits(), second.exits());
}

TEST_P(GeneratorTest, DifferentSeedsGiveDifferentMazes) {
  EXPECT_NE(
    walls_of(generate(GetParam(), 25, 18, options(1, 3))),
    walls_of(generate(GetParam(), 25, 18, options(2, 3))));
}

TEST_P(GeneratorTest, WithoutASeedEveryMazeIsDifferent) {
  EXPECT_NE(
    walls_of(generate(GetParam(), 25, 18, options(std::nullopt, 3))),
    walls_of(generate(GetParam(), 25, 18, options(std::nullopt, 3))));
}

// ---------- reusing the generator and the grid ----------

TEST_P(GeneratorTest, ReusingTheGridDoesNotKeepOldExits) {
  MazeGrid grid(20, 20);
  MazeGenerator generator{GetParam()};
  for (uint64_t seed = 1; seed <= 4; ++seed) {
    generator.generate(grid, options(seed, 3));
    EXPECT_EQ(grid.exits().size(), 3u) << "call " << seed;
    EXPECT_EQ(analyze(grid).border_openings, 3) << "call " << seed;
  }
}

TEST_P(GeneratorTest, ReusingTheGridGivesTheSameMazeAsAFreshOne) {
  MazeGrid reused(20, 20);
  MazeGenerator generator{GetParam()};
  generator.generate(reused, options(1, 5));
  generator.generate(reused, options(2, 2));

  const MazeGrid fresh = generate(GetParam(), 20, 20, options(2, 2));
  EXPECT_EQ(walls_of(reused), walls_of(fresh));
  EXPECT_EQ(reused.exits(), fresh.exits());
}

// A call without a seed used to keep using the seed of the previous call
TEST_P(GeneratorTest, SeedIsNotRememberedBetweenCalls) {
  MazeGenerator generator{GetParam()};
  const MazeGrid seeded = generator.generate(25, 18, options(42, 0));
  const MazeGrid first = generator.generate(25, 18, options(std::nullopt, 0));
  const MazeGrid second = generator.generate(25, 18, options(std::nullopt, 0));

  EXPECT_NE(walls_of(first), walls_of(seeded));
  EXPECT_NE(walls_of(first), walls_of(second));
}

// ---------- awkward sizes ----------

TEST_P(GeneratorTest, HandlesTinyAndThinGrids) {
  const std::pair<int, int> sizes[] = {
    {1, 1}, {1, 2}, {2, 1}, {1, 8}, {8, 1}, {2, 2}, {3, 3}, {4, 4}, {5, 5}, {6, 6}, {3, 20}, {20, 3}
  };
  for (const auto & [width, height] : sizes) {
    for (int level : {0, 10}) {
      MazeStrategyOptions opts = options(3, 0);
      opts.room_level = level;
      opts.cave_level = level;
      opts.difficulty = level;
      MazeGrid grid(width, height);
      MazeGenerator generator{GetParam()};
      ASSERT_NO_THROW(generator.generate(grid, opts)) << test::size_name(width, height);
      const auto stats = analyze(grid);
      EXPECT_LE(stats.regions, 1) << test::size_name(width, height);
      EXPECT_EQ(stats.asymmetric_walls, 0) << test::size_name(width, height);
      EXPECT_EQ(stats.border_openings, 0) << test::size_name(width, height);
    }
  }
}

TEST_P(GeneratorTest, CanFillEveryExitSlot) {
  // 6x4: 4 + 4 cells on the top and bottom rows and 2 + 2 on the sides, corners excluded
  const MazeGrid grid = generate(GetParam(), 6, 4, options(1, 12));
  EXPECT_EQ(grid.exits().size(), 12u);
  EXPECT_EQ(analyze(grid).border_openings, 12);
  EXPECT_LE(analyze(grid).regions, 1);
}

INSTANTIATE_TEST_SUITE_P(
  AllAlgorithms, GeneratorTest, ::testing::ValuesIn(test::every_algorithm()),
  algorithm_param_name);

// ---------- difficulty ----------

class DifficultyTest : public ::testing::TestWithParam<MazeAlgorithm>
{
protected:
  test::MazeStats maze(int difficulty, uint64_t seed) const
  {
    MazeStrategyOptions opts = options(seed, 0);
    opts.difficulty = difficulty;
    opts.rooms = false;
    return analyze(generate(GetParam(), 30, 30, opts));
  }
};

TEST_P(DifficultyTest, TenKeepsThePerfectMaze) {
  for (uint64_t seed = 1; seed <= 5; ++seed) {
    EXPECT_TRUE(test::is_perfect(maze(10, seed))) << "seed " << seed;
  }
}

TEST_P(DifficultyTest, ZeroRemovesEveryDeadEnd) {
  for (uint64_t seed = 1; seed <= 5; ++seed) {
    const auto stats = maze(0, seed);
    EXPECT_EQ(stats.dead_ends, 0) << "seed " << seed;
    EXPECT_EQ(stats.regions, 1) << "seed " << seed;
    EXPECT_EQ(stats.closed_cells, 0) << "seed " << seed;
    EXPECT_GT(stats.passages, stats.cells - 1) << "seed " << seed;
  }
}

TEST_P(DifficultyTest, LowerDifficultyMeansFewerDeadEnds) {
  auto dead_ends = [this](int difficulty) {
      int total = 0;
      for (uint64_t seed = 1; seed <= 10; ++seed) {
        total += maze(difficulty, seed).dead_ends;
      }
      return total;
    };
  int previous = dead_ends(10);
  for (int difficulty : {7, 4, 1, 0}) {
    const int current = dead_ends(difficulty);
    EXPECT_LT(current, previous) << "difficulty " << difficulty;
    previous = current;
  }
}

TEST_P(DifficultyTest, NeverDisconnectsOrOpensTheBorder) {
  for (int difficulty = 0; difficulty <= 10; ++difficulty) {
    const auto stats = maze(difficulty, 3);
    EXPECT_EQ(stats.regions, 1) << "difficulty " << difficulty;
    EXPECT_EQ(stats.border_openings, 0) << "difficulty " << difficulty;
    EXPECT_EQ(stats.asymmetric_walls, 0) << "difficulty " << difficulty;
  }
}

INSTANTIATE_TEST_SUITE_P(
  PerfectAlgorithms, DifficultyTest, ::testing::ValuesIn(test::perfect_algorithms()),
  algorithm_param_name);

TEST(Difficulty, OnlyAddsPassagesToThePerfectMaze) {
  MazeStrategyOptions perfect = options(4, 0);
  perfect.rooms = false;
  MazeStrategyOptions braided = perfect;
  braided.difficulty = 3;
  const auto before = walls_of(generate(MazeAlgorithm::Kruskal, 25, 25, perfect));
  const auto after = walls_of(generate(MazeAlgorithm::Kruskal, 25, 25, braided));
  ASSERT_EQ(before.size(), after.size());
  for (size_t i = 0; i < before.size(); ++i) {
    EXPECT_EQ(after[i] & ~before[i], 0) << "cell " << i << " gained a wall";
  }
  EXPECT_NE(before, after);
}

TEST(Difficulty, IsIgnoredByTheCaveGenerator) {
  MazeStrategyOptions hard = options(4, 0);
  MazeStrategyOptions easy = hard;
  easy.difficulty = 0;
  EXPECT_EQ(
    walls_of(generate(MazeAlgorithm::CellularAutomaton, 25, 25, hard)),
    walls_of(generate(MazeAlgorithm::CellularAutomaton, 25, 25, easy)));
}

// A corridor one cell wide has dead ends at both tips that no wall can remove
TEST(Difficulty, LeavesTheTipsOfACorridorMaze) {
  MazeStrategyOptions opts = options(4, 0);
  opts.difficulty = 0;
  const auto stats = analyze(generate(MazeAlgorithm::DFS, 9, 1, opts));
  EXPECT_EQ(stats.dead_ends, 2);
  EXPECT_EQ(stats.passages, 8);
}

// ---------- fixed exits ----------

TEST(FixedExits, AreOpenedWhereAsked) {
  MazeStrategyOptions opts = options(1, 0);
  opts.fixed_exits = {{0, 3, Dir::W}, {9, 6, Dir::E}, {4, 0, Dir::N}, {5, 9, Dir::S}};
  const MazeGrid grid = generate(MazeAlgorithm::DFS, 10, 10, opts);

  EXPECT_EQ(grid.exits(), opts.fixed_exits);
  EXPECT_EQ(analyze(grid).border_openings, 4);
  for (const Exit & exit : grid.exits()) {
    expect_valid_exit(grid, exit);
  }
}

TEST(FixedExits, AreAllowedOnACorner) {
  MazeStrategyOptions opts = options(1, 0);
  opts.fixed_exits = {{0, 0, Dir::N}, {0, 0, Dir::W}, {9, 9, Dir::E}};
  const MazeGrid grid = generate(MazeAlgorithm::Kruskal, 10, 10, opts);
  EXPECT_EQ(grid.exits().size(), 3u);
  EXPECT_EQ(analyze(grid).border_openings, 3);
}

TEST(FixedExits, ComeBeforeTheRandomOnes) {
  MazeStrategyOptions opts = options(1, 3);
  opts.fixed_exits = {{0, 4, Dir::W}};
  const MazeGrid grid = generate(MazeAlgorithm::Prim, 10, 10, opts);
  ASSERT_EQ(grid.exits().size(), 4u);
  EXPECT_EQ(grid.exits()[0], (Exit{0, 4, Dir::W}));
  EXPECT_EQ(analyze(grid).border_openings, 4);
}

// A random exit must never land on a slot that a fixed exit already uses
TEST(FixedExits, AreNeverRepeatedByARandomOne) {
  MazeStrategyOptions opts = options(1, 11);  // 12 slots on a 6x4 grid, one taken
  opts.fixed_exits = {{0, 2, Dir::W}};
  const MazeGrid grid = generate(MazeAlgorithm::DFS, 6, 4, opts);
  const std::set<Exit> unique(grid.exits().begin(), grid.exits().end());
  EXPECT_EQ(grid.exits().size(), 12u);
  EXPECT_EQ(unique.size(), 12u);
  EXPECT_EQ(analyze(grid).border_openings, 12);
}

TEST(FixedExits, WorkOnASingleCell) {
  MazeStrategyOptions opts = options(1, 0);
  opts.fixed_exits = {{0, 0, Dir::N}, {0, 0, Dir::S}};
  const MazeGrid grid = generate(MazeAlgorithm::DFS, 1, 1, opts);
  EXPECT_EQ(grid.exits().size(), 2u);
  EXPECT_EQ(analyze(grid).border_openings, 2);
}

// On a cave an exit can land on solid rock: the generator has to dig it into the cave
TEST(FixedExits, AreConnectedToTheCaveEvenThroughRock) {
  for (uint64_t seed = 1; seed <= 10; ++seed) {
    MazeStrategyOptions opts = options(seed, 0);
    opts.cave_level = 10;  // as much rock as it gets
    opts.fixed_exits = {{0, 0, Dir::N}, {29, 0, Dir::E}, {0, 29, Dir::W}, {29, 29, Dir::S}};
    const MazeGrid grid = generate(MazeAlgorithm::CellularAutomaton, 30, 30, opts);
    EXPECT_EQ(grid.exits().size(), 4u) << "seed " << seed;
    EXPECT_EQ(analyze(grid).regions, 1) << "seed " << seed;
  }
}

// ---------- options that must be rejected ----------

TEST(InvalidOptions, DifficultyOutOfRange) {
  for (int difficulty : {-1, 11, -1000, 1000}) {
    MazeStrategyOptions opts = options(1);
    opts.difficulty = difficulty;
    EXPECT_NE(rejection(opts).find("difficulty"), std::string::npos) << difficulty;
  }
}

TEST(InvalidOptions, RoomLevelOutOfRange) {
  for (int level : {-1, 11}) {
    MazeStrategyOptions opts = options(1);
    opts.room_level = level;
    EXPECT_NE(rejection(opts).find("room_level"), std::string::npos) << level;
  }
}

TEST(InvalidOptions, CaveLevelOutOfRange) {
  for (int level : {-1, 11}) {
    MazeStrategyOptions opts = options(1);
    opts.cave_level = level;
    EXPECT_NE(rejection(opts).find("cave_level"), std::string::npos) << level;
  }
}

TEST(InvalidOptions, ProbabilitiesOutOfRange) {
  const double bad_values[] = {
    -0.01, 1.01, -5.0, 5.0, std::numeric_limits<double>::quiet_NaN(),
    std::numeric_limits<double>::infinity()
  };
  for (double value : bad_values) {
    MazeStrategyOptions growing_tree = options(1);
    growing_tree.growing_tree_newest_ratio = value;
    EXPECT_NE(rejection(growing_tree).find("growing_tree_newest_ratio"), std::string::npos)
      << value;

    MazeStrategyOptions sidewinder = options(1);
    sidewinder.sidewinder_east_probability = value;
    EXPECT_NE(rejection(sidewinder).find("sidewinder_east_probability"), std::string::npos)
      << value;

    MazeStrategyOptions ellers = options(1);
    ellers.ellers_join_probability = value;
    EXPECT_NE(rejection(ellers).find("ellers_join_probability"), std::string::npos) << value;
  }
}

TEST(InvalidOptions, NegativeNumberOfExits) {
  EXPECT_NE(rejection(options(1, -1)).find("num_random_exits"), std::string::npos);
  EXPECT_NE(rejection(options(1, -1000)).find("num_random_exits"), std::string::npos);
}

TEST(InvalidOptions, MoreRandomExitsThanTheGridHasRoomFor) {
  EXPECT_EQ(rejection(options(1, 12), 6, 4), "");
  EXPECT_NE(rejection(options(1, 13), 6, 4).find("num_random_exits"), std::string::npos);
  // A grid made only of corner cells has no slot at all
  EXPECT_EQ(rejection(options(1, 0), 2, 2), "");
  EXPECT_NE(rejection(options(1, 1), 2, 2).find("num_random_exits"), std::string::npos);
  EXPECT_NE(rejection(options(1, 1), 1, 1).find("num_random_exits"), std::string::npos);
}

TEST(InvalidOptions, FixedExitsTakeSlotsAwayFromTheRandomOnes) {
  MazeStrategyOptions opts = options(1, 12);
  opts.fixed_exits = {{0, 2, Dir::W}};
  EXPECT_NE(rejection(opts, 6, 4).find("num_random_exits"), std::string::npos);
  // A fixed exit on a corner uses no slot
  opts.fixed_exits = {{0, 0, Dir::W}};
  EXPECT_EQ(rejection(opts, 6, 4), "");
}

TEST(InvalidOptions, FixedExitInsideTheMaze) {
  MazeStrategyOptions opts = options(1, 0);
  opts.fixed_exits = {{4, 4, Dir::N}};
  EXPECT_NE(rejection(opts).find("does not point outside"), std::string::npos);
}

TEST(InvalidOptions, FixedExitPointingIntoTheMaze) {
  for (const Exit & exit : {Exit{0, 3, Dir::E}, Exit{4, 0, Dir::S}, Exit{9, 6, Dir::N}}) {
    MazeStrategyOptions opts = options(1, 0);
    opts.fixed_exits = {exit};
    EXPECT_NE(rejection(opts).find("does not point outside"), std::string::npos)
      << ::testing::PrintToString(exit);
  }
}

// Coordinates off the grid used to be stored as exits and later used as indices
TEST(InvalidOptions, FixedExitOutsideTheGrid) {
  const Exit outside[] = {
    {-1, 0, Dir::N}, {0, -1, Dir::W}, {10, 0, Dir::N}, {0, 10, Dir::S},
    {-1, -1, Dir::N}, {500, 500, Dir::E}, {10, 9, Dir::E}, {9, 10, Dir::S}
  };
  for (const Exit & exit : outside) {
    MazeStrategyOptions opts = options(1, 0);
    opts.fixed_exits = {exit};
    EXPECT_NE(rejection(opts).find("outside the grid"), std::string::npos)
      << ::testing::PrintToString(exit);
  }
}

TEST(InvalidOptions, FixedExitWithAMadeUpDirection) {
  for (int raw : {0, 3, 16, 255}) {
    MazeStrategyOptions opts = options(1, 0);
    opts.fixed_exits = {{0, 3, static_cast<Dir>(raw)}};
    EXPECT_NE(rejection(opts).find("direction"), std::string::npos) << raw;
  }
}

TEST(InvalidOptions, RepeatedFixedExit) {
  MazeStrategyOptions opts = options(1, 0);
  opts.fixed_exits = {{0, 3, Dir::W}, {9, 3, Dir::E}, {0, 3, Dir::W}};
  const std::string message = rejection(opts);
  EXPECT_NE(message.find("repeated"), std::string::npos);
  EXPECT_NE(message.find("#2"), std::string::npos) << "the message should say which one";
}

TEST(InvalidOptions, TheMessageNamesTheBadValue) {
  MazeStrategyOptions opts = options(1);
  opts.difficulty = 42;
  EXPECT_NE(rejection(opts).find("42"), std::string::npos);
}

// A rejected call must leave the previous maze exactly as it was
TEST(InvalidOptions, LeaveTheGridUntouched) {
  MazeGrid grid(10, 10);
  MazeGenerator generator{MazeAlgorithm::DFS};
  generator.generate(grid, options(1, 2));
  const auto walls = walls_of(grid);
  const auto exits = grid.exits();

  MazeStrategyOptions bad = options(2, 2);
  bad.difficulty = 99;
  EXPECT_THROW(generator.generate(grid, bad), std::invalid_argument);
  bad = options(2, 2);
  bad.fixed_exits = {{50, 50, Dir::N}};
  EXPECT_THROW(generator.generate(grid, bad), std::invalid_argument);

  EXPECT_EQ(walls_of(grid), walls);
  EXPECT_EQ(grid.exits(), exits);
}

TEST(InvalidOptions, BadGridSizeIsRejectedByTheOverloadThatMakesTheGrid) {
  MazeGenerator generator;
  EXPECT_THROW(generator.generate(0, 5, options(1)), std::invalid_argument);
  EXPECT_THROW(generator.generate(5, -2, options(1)), std::invalid_argument);
}

TEST(ValidOptions, TheLimitsOfEveryRangeAreAccepted) {
  MazeStrategyOptions low = options(1, 0);
  low.difficulty = 0;
  low.room_level = 0;
  low.cave_level = 0;
  low.growing_tree_newest_ratio = 0.0;
  low.sidewinder_east_probability = 0.0;
  low.ellers_join_probability = 0.0;
  EXPECT_EQ(rejection(low), "");

  MazeStrategyOptions high = options(1, 0);
  high.difficulty = 10;
  high.room_level = 10;
  high.cave_level = 10;
  high.growing_tree_newest_ratio = 1.0;
  high.sidewinder_east_probability = 1.0;
  high.ellers_join_probability = 1.0;
  EXPECT_EQ(rejection(high), "");
}

TEST(ValidOptions, DefaultsGiveAPerfectMazeWithOneExit) {
  const MazeStrategyOptions defaults;
  EXPECT_EQ(defaults.difficulty, 10);
  EXPECT_EQ(defaults.num_random_exits, 1);
  EXPECT_FALSE(defaults.seed.has_value());
  EXPECT_TRUE(defaults.fixed_exits.empty());
  EXPECT_TRUE(defaults.rooms);

  // Too small for a room, so nothing but the algorithm and the exit
  const MazeGrid grid = generate(MazeAlgorithm::DFS, 4, 4, defaults);
  EXPECT_EQ(grid.exits().size(), 1u);
  EXPECT_TRUE(test::is_perfect(analyze(grid)));
}

// ---------- rooms ----------

TEST(Rooms, AddLoopsToAPerfectMaze) {
  int mazes_with_loops = 0;
  for (uint64_t seed = 1; seed <= 10; ++seed) {
    const auto stats = analyze(generate(MazeAlgorithm::Kruskal, 30, 30, options(seed, 0)));
    EXPECT_GE(stats.passages, stats.cells - 1);
    EXPECT_EQ(stats.regions, 1);
    EXPECT_EQ(stats.closed_cells, 0);
    if (stats.passages > stats.cells - 1) {
      ++mazes_with_loops;
    }
  }
  EXPECT_GT(mazes_with_loops, 0);
}

TEST(Rooms, CanBeSwitchedOff) {
  for (MazeAlgorithm algorithm : test::perfect_algorithms()) {
    MazeStrategyOptions opts = options(3, 0);
    opts.rooms = false;
    opts.room_level = 10;
    EXPECT_TRUE(test::is_perfect(analyze(generate(algorithm, 30, 30, opts))))
      << test::algorithm_name(algorithm);
  }
}

TEST(Rooms, AHigherLevelOpensMoreSpace) {
  auto passages = [](int level) {
      int total = 0;
      for (uint64_t seed = 1; seed <= 10; ++seed) {
        MazeStrategyOptions opts = options(seed, 0);
        opts.room_level = level;
        total += analyze(generate(MazeAlgorithm::Kruskal, 40, 40, opts)).passages;
      }
      return total;
    };
  EXPECT_LT(passages(0), passages(5));
  EXPECT_LT(passages(5), passages(10));
}

TEST(Rooms, AreSkippedWhenTheMazeIsTooSmallForOne) {
  for (MazeAlgorithm algorithm : test::perfect_algorithms()) {
    EXPECT_TRUE(test::is_perfect(analyze(generate(algorithm, 4, 4, options(1, 0)))))
      << test::algorithm_name(algorithm);
  }
}

// Recursive Division rebuilds every wall, so rooms carved before it would be lost
TEST(Rooms, SurviveRecursiveDivision) {
  int mazes_with_loops = 0;
  for (uint64_t seed = 1; seed <= 10; ++seed) {
    const auto stats = analyze(
      generate(MazeAlgorithm::RecursiveDivision, 30, 30, options(seed, 0)));
    if (stats.passages > stats.cells - 1) {
      ++mazes_with_loops;
    }
  }
  EXPECT_GT(mazes_with_loops, 0);
}

TEST(Rooms, AreNotAddedToACave) {
  // With no exits, generate() on a cave has to match the algorithm run on its own
  for (uint64_t seed = 1; seed <= 5; ++seed) {
    MazeStrategyOptions opts = options(seed, 0);
    opts.cave_level = 4;
    opts.room_level = 10;
    EXPECT_EQ(
      walls_of(generate(MazeAlgorithm::CellularAutomaton, 30, 30, opts)),
      walls_of(test::run_strategy(MazeAlgorithm::CellularAutomaton, 30, 30, seed, opts)));
  }
}

// ---------- phases do not share a random sequence ----------

// With one sequence for everything, switching the rooms off would shift the numbers
// the exits get. Each phase has its own, so the exits stay where they were
TEST(Phases, ExitsDoNotDependOnTheRooms) {
  MazeStrategyOptions with_rooms = options(21, 4);
  MazeStrategyOptions without_rooms = with_rooms;
  without_rooms.rooms = false;
  EXPECT_EQ(
    generate(MazeAlgorithm::DFS, 30, 30, with_rooms).exits(),
    generate(MazeAlgorithm::DFS, 30, 30, without_rooms).exits());
}

TEST(Phases, ExitsDoNotDependOnTheAlgorithmOrTheDifficulty) {
  MazeStrategyOptions opts = options(21, 4);
  const auto exits = generate(MazeAlgorithm::DFS, 30, 30, opts).exits();
  EXPECT_EQ(generate(MazeAlgorithm::Wilson, 30, 30, opts).exits(), exits);
  opts.difficulty = 2;
  EXPECT_EQ(generate(MazeAlgorithm::DFS, 30, 30, opts).exits(), exits);
}

TEST(Phases, TheAlgorithmDoesNotDependOnRoomsOrExits) {
  MazeStrategyOptions opts = options(21, 0);
  opts.rooms = false;
  const auto walls = walls_of(generate(MazeAlgorithm::Prim, 30, 30, opts));
  EXPECT_EQ(walls, walls_of(test::run_strategy(MazeAlgorithm::Prim, 30, 30, 21)));
}

// ---------- choosing the algorithm ----------

TEST(GeneratorAlgorithm, DefaultIsDFS) {
  MazeGenerator generator;
  EXPECT_EQ(generator.algorithm(), MazeAlgorithm::DFS);
  EXPECT_EQ(
    walls_of(generator.generate(15, 15, options(8))),
    walls_of(generate(MazeAlgorithm::DFS, 15, 15, options(8))));
}

TEST(GeneratorAlgorithm, SetAlgorithmSwitchesIt) {
  MazeGenerator generator{MazeAlgorithm::DFS};
  MazeGrid grid(15, 15);
  for (MazeAlgorithm algorithm : all_algorithms()) {
    generator.set_algorithm(algorithm);
    EXPECT_EQ(generator.algorithm(), algorithm);
    generator.generate(grid, options(8));
    EXPECT_EQ(walls_of(grid), walls_of(generate(algorithm, 15, 15, options(8))))
      << test::algorithm_name(algorithm);
  }
}

TEST(GeneratorAlgorithm, EachAlgorithmGivesItsOwnMaze) {
  std::set<std::vector<uint8_t>> mazes;
  for (MazeAlgorithm algorithm : all_algorithms()) {
    mazes.insert(walls_of(generate(algorithm, 20, 20, options(8))));
  }
  EXPECT_EQ(mazes.size(), all_algorithms().size());
}

TEST(GeneratorAlgorithm, AnUnknownAlgorithmIsRejected) {
  const auto unknown = static_cast<MazeAlgorithm>(9999);
  EXPECT_THROW(MazeGenerator{unknown}, std::invalid_argument);

  MazeGenerator generator{MazeAlgorithm::Prim};
  EXPECT_THROW(generator.set_algorithm(unknown), std::invalid_argument);
  EXPECT_EQ(generator.algorithm(), MazeAlgorithm::Prim) << "a failed switch must change nothing";
  EXPECT_NO_THROW(generator.generate(8, 8, options(1)));
}

TEST(GeneratorAlgorithm, GeneratorsCanBeMoved) {
  MazeGenerator original{MazeAlgorithm::Kruskal};
  MazeGenerator moved = std::move(original);
  EXPECT_EQ(moved.algorithm(), MazeAlgorithm::Kruskal);
  EXPECT_EQ(
    walls_of(moved.generate(12, 12, options(8))),
    walls_of(generate(MazeAlgorithm::Kruskal, 12, 12, options(8))));
}

TEST(GeneratorAlgorithm, BothGenerateOverloadsAgree) {
  MazeGenerator generator{MazeAlgorithm::Wilson};
  MazeGrid in_place(14, 9);
  generator.generate(in_place, options(8, 3));
  const MazeGrid returned = generator.generate(14, 9, options(8, 3));
  EXPECT_EQ(walls_of(in_place), walls_of(returned));
  EXPECT_EQ(in_place.exits(), returned.exits());
}
