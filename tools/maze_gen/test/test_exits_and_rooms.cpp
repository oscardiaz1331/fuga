// MazeExitGenerator, RoomGenerator and the dead-end braider used directly, without the
// rest of the pipeline.
#include <gtest/gtest.h>
#include <set>
#include <utility>
#include <vector>
#include "dead_end_braider.hpp"
#include "maze_exit_generator.hpp"
#include "maze_test_utils.hpp"
#include "room_generator.hpp"

using namespace maze_gen;  // NOLINT(build/namespaces)
using maze_gen::test::analyze;
using maze_gen::test::walls_of;

namespace
{

MazeStrategyOptions exit_options(int random_exits, std::vector<Exit> fixed = {})
{
  MazeStrategyOptions options;
  options.num_random_exits = random_exits;
  options.fixed_exits = std::move(fixed);
  return options;
}

MazeGrid with_exits(int width, int height, uint64_t seed, const MazeStrategyOptions & options)
{
  MazeGrid grid(width, height);
  Random rng(seed);
  MazeExitGenerator().generate_exits(grid, rng, options);
  return grid;
}

MazeGrid with_rooms(int width, int height, uint64_t seed, int level)
{
  MazeGrid grid(width, height);
  Random rng(seed);
  RoomGenerator().generate_rooms(grid, rng, level);
  return grid;
}

}  // namespace

// ===================== exit slots =====================

TEST(ExitSlots, AreEveryOuterSideOfTheNonCornerBorderCells) {
  const MazeGrid grid(8, 6);
  const auto slots = MazeExitGenerator::random_exit_slots(grid);
  // 6 + 6 on the top and bottom rows, 4 + 4 on the sides
  EXPECT_EQ(slots.size(), 20u);
  EXPECT_EQ(std::set<Exit>(slots.begin(), slots.end()).size(), 20u);
  for (const Exit & slot : slots) {
    const bool corner = (slot.x == 0 || slot.x == 7) && (slot.y == 0 || slot.y == 5);
    EXPECT_FALSE(corner) << ::testing::PrintToString(slot);
    const auto [nx, ny] = MazeGrid::neighbor(slot.x, slot.y, slot.dir);
    EXPECT_TRUE(grid.in_bounds(slot.x, slot.y));
    EXPECT_FALSE(grid.in_bounds(nx, ny)) << ::testing::PrintToString(slot);
  }
}

TEST(ExitSlots, GridsMadeOnlyOfCornersHaveNone) {
  const std::pair<int, int> sizes[] = {{1, 1}, {2, 1}, {1, 2}, {2, 2}};
  for (const auto & [width, height] : sizes) {
    EXPECT_TRUE(MazeExitGenerator::random_exit_slots(MazeGrid(width, height)).empty())
      << test::size_name(width, height);
  }
}

TEST(ExitSlots, AThinGridUsesBothSidesOfItsMiddleCells) {
  const auto slots = MazeExitGenerator::random_exit_slots(MazeGrid(1, 6));
  EXPECT_EQ(slots.size(), 8u);  // 4 middle cells, east and west each
  for (const Exit & slot : slots) {
    EXPECT_TRUE(slot.dir == Dir::E || slot.dir == Dir::W);
    EXPECT_GT(slot.y, 0);
    EXPECT_LT(slot.y, 5);
  }
}

// ===================== exits =====================

TEST(ExitGenerator, OpensOneBorderWallPerExitAndNothingElse) {
  const MazeGrid grid = with_exits(8, 6, 1, exit_options(5));
  const auto stats = analyze(grid);
  EXPECT_EQ(grid.exits().size(), 5u);
  EXPECT_EQ(stats.border_openings, 5);
  EXPECT_EQ(stats.passages, 0);
}

TEST(ExitGenerator, PutsEachExitOnTheSideItPointsTo) {
  const MazeGrid grid = with_exits(8, 6, 1, exit_options(20));
  ASSERT_EQ(grid.exits().size(), 20u);
  for (const Exit & exit : grid.exits()) {
    switch (exit.dir) {
      case Dir::N: EXPECT_EQ(exit.y, 0); break;
      case Dir::S: EXPECT_EQ(exit.y, grid.height() - 1); break;
      case Dir::W: EXPECT_EQ(exit.x, 0); break;
      case Dir::E: EXPECT_EQ(exit.x, grid.width() - 1); break;
    }
  }
}

TEST(ExitGenerator, SameSeedPicksTheSameExits) {
  EXPECT_EQ(
    with_exits(12, 9, 31, exit_options(4)).exits(),
    with_exits(12, 9, 31, exit_options(4)).exits());
}

TEST(ExitGenerator, DifferentSeedsPickDifferentExits) {
  EXPECT_NE(
    with_exits(12, 9, 1, exit_options(4)).exits(),
    with_exits(12, 9, 2, exit_options(4)).exits());
}

TEST(ExitGenerator, ZeroAddsNothing) {
  const MazeGrid grid = with_exits(8, 6, 1, exit_options(0));
  EXPECT_TRUE(grid.exits().empty());
  EXPECT_EQ(walls_of(grid), walls_of(MazeGrid(8, 6)));
}

TEST(ExitGenerator, FixedExitsComeFirstAndInTheirOrder) {
  const std::vector<Exit> fixed = {{7, 5, Dir::S}, {0, 2, Dir::W}, {0, 0, Dir::N}};
  const MazeGrid grid = with_exits(8, 6, 1, exit_options(3, fixed));
  ASSERT_EQ(grid.exits().size(), 6u);
  for (size_t i = 0; i < fixed.size(); ++i) {
    EXPECT_EQ(grid.exits()[i], fixed[i]);
  }
  EXPECT_EQ(analyze(grid).border_openings, 6);
}

TEST(ExitGenerator, RandomExitsAvoidTheSlotsOfTheFixedOnes) {
  // Ask for every remaining slot: none may repeat the fixed exit
  const MazeGrid grid = with_exits(8, 6, 1, exit_options(19, {{0, 2, Dir::W}}));
  EXPECT_EQ(grid.exits().size(), 20u);
  EXPECT_EQ(std::set<Exit>(grid.exits().begin(), grid.exits().end()).size(), 20u);
  EXPECT_EQ(analyze(grid).border_openings, 20);
}

// ===================== rooms =====================

TEST(RoomGenerator, LeavesSmallMazesUntouched) {
  const std::pair<int, int> sizes[] = {
    {1, 1}, {2, 2}, {3, 3}, {4, 4}, {4, 30}, {30, 4}, {1, 50}
  };
  for (const auto & [width, height] : sizes) {
    for (int level : {0, 5, 10}) {
      EXPECT_EQ(walls_of(with_rooms(width, height, 1, level)), walls_of(MazeGrid(width, height)))
        << test::size_name(width, height) << " level " << level;
    }
  }
}

// Sizes just big enough for a room are where the placement arithmetic is tightest
TEST(RoomGenerator, HandlesMazesBarelyBigEnoughForARoom) {
  for (int size = 5; size <= 12; ++size) {
    for (int level = 0; level <= 10; ++level) {
      for (uint64_t seed = 1; seed <= 5; ++seed) {
        const auto stats = analyze(with_rooms(size, size, seed, level));
        EXPECT_EQ(stats.border_openings, 0) << size << " level " << level;
        EXPECT_EQ(stats.asymmetric_walls, 0) << size << " level " << level;
      }
    }
  }
}

TEST(RoomGenerator, CarvesOpenAreasInALargeMaze) {
  const MazeGrid grid = with_rooms(40, 40, 1, 5);
  const auto stats = analyze(grid);
  EXPECT_GT(stats.passages, 0);
  EXPECT_EQ(stats.border_openings, 0);
  EXPECT_EQ(stats.asymmetric_walls, 0);

  // A room is a block of cells with no walls between them: at least one 2x2 block is open
  bool found_open_block = false;
  for (int y = 0; y + 1 < grid.height() && !found_open_block; ++y) {
    for (int x = 0; x + 1 < grid.width() && !found_open_block; ++x) {
      found_open_block = !grid.has_wall(x, y, Dir::E) && !grid.has_wall(x, y, Dir::S) &&
        !grid.has_wall(x + 1, y + 1, Dir::N) && !grid.has_wall(x + 1, y + 1, Dir::W);
    }
  }
  EXPECT_TRUE(found_open_block);
}

// Rooms keep a margin, so the outer ring of cells is only ever reached through a door
TEST(RoomGenerator, KeepsRoomsAwayFromTheBorder) {
  for (uint64_t seed = 1; seed <= 20; ++seed) {
    const MazeGrid grid = with_rooms(30, 20, seed, 10);
    for (int x = 0; x + 1 < grid.width(); ++x) {
      EXPECT_TRUE(grid.has_wall(x, 0, Dir::E)) << "seed " << seed << ", top row x=" << x;
      EXPECT_TRUE(grid.has_wall(x, grid.height() - 1, Dir::E))
        << "seed " << seed << ", bottom row x=" << x;
    }
    for (int y = 0; y + 1 < grid.height(); ++y) {
      EXPECT_TRUE(grid.has_wall(0, y, Dir::S)) << "seed " << seed << ", left column y=" << y;
      EXPECT_TRUE(grid.has_wall(grid.width() - 1, y, Dir::S))
        << "seed " << seed << ", right column y=" << y;
    }
  }
}

TEST(RoomGenerator, SameSeedGivesTheSameRooms) {
  EXPECT_EQ(walls_of(with_rooms(30, 30, 5, 4)), walls_of(with_rooms(30, 30, 5, 4)));
}

TEST(RoomGenerator, DifferentSeedsGiveDifferentRooms) {
  EXPECT_NE(walls_of(with_rooms(30, 30, 1, 4)), walls_of(with_rooms(30, 30, 2, 4)));
}

TEST(RoomGenerator, OnlyRemovesWalls) {
  // Rooms run after the algorithm: they must never close a passage it carved
  MazeGrid grid = test::run_strategy(MazeAlgorithm::Kruskal, 30, 30, 3);
  const auto before = walls_of(grid);
  Random rng(3);
  RoomGenerator().generate_rooms(grid, rng, 6);
  const auto after = walls_of(grid);
  for (size_t i = 0; i < before.size(); ++i) {
    EXPECT_EQ(after[i] & ~before[i], 0) << "cell " << i << " gained a wall";
  }
  EXPECT_NE(before, after);
}

// ===================== dead-end braider =====================

TEST(Braider, ZeroLeavesTheMazeAlone) {
  MazeGrid grid = test::run_strategy(MazeAlgorithm::Prim, 20, 20, 3);
  const auto before = walls_of(grid);
  Random rng(1);
  braid(grid, rng, 0.0);
  EXPECT_EQ(walls_of(grid), before);
}

TEST(Braider, OneRemovesEveryDeadEnd) {
  for (MazeAlgorithm algorithm : test::perfect_algorithms()) {
    MazeGrid grid = test::run_strategy(algorithm, 20, 20, 3);
    ASSERT_GT(analyze(grid).dead_ends, 0);
    Random rng(1);
    braid(grid, rng, 1.0);
    EXPECT_EQ(analyze(grid).dead_ends, 0) << test::algorithm_name(algorithm);
  }
}

TEST(Braider, RemovesRoughlyTheRequestedShare) {
  int before = 0;
  int after = 0;
  for (uint64_t seed = 1; seed <= 20; ++seed) {
    MazeGrid grid = test::run_strategy(MazeAlgorithm::Kruskal, 30, 30, seed);
    before += analyze(grid).dead_ends;
    Random rng(seed);
    braid(grid, rng, 0.5);
    after += analyze(grid).dead_ends;
  }
  // Half are picked, and opening towards another dead end removes that one as well
  EXPECT_LT(after, before * 0.5);
  EXPECT_GT(after, before * 0.2);
}

TEST(Braider, OnlyRemovesWallsAndKeepsTheBorderClosed) {
  MazeGrid grid = test::run_strategy(MazeAlgorithm::DFS, 25, 25, 3);
  const auto before = walls_of(grid);
  Random rng(1);
  braid(grid, rng, 1.0);
  const auto after = walls_of(grid);
  for (size_t i = 0; i < before.size(); ++i) {
    EXPECT_EQ(after[i] & ~before[i], 0) << "cell " << i << " gained a wall";
  }
  const auto stats = analyze(grid);
  EXPECT_EQ(stats.border_openings, 0);
  EXPECT_EQ(stats.asymmetric_walls, 0);
  EXPECT_EQ(stats.regions, 1);
}

TEST(Braider, LeavesSolidRockAlone) {
  // A fully walled grid has no dead ends: its cells have no passage at all
  MazeGrid grid(6, 6);
  Random rng(1);
  braid(grid, rng, 1.0);
  EXPECT_EQ(walls_of(grid), walls_of(MazeGrid(6, 6)));
}

TEST(Braider, CannotOpenTheTipOfACorridor) {
  MazeGrid grid(5, 1);
  for (int x = 0; x < 4; ++x) {
    grid.carve(x, 0, Dir::E);
  }
  const auto before = walls_of(grid);
  Random rng(1);
  braid(grid, rng, 1.0);
  EXPECT_EQ(walls_of(grid), before);
}

TEST(Braider, SameSeedGivesTheSameResult) {
  MazeGrid first = test::run_strategy(MazeAlgorithm::Kruskal, 20, 20, 3);
  MazeGrid second = test::run_strategy(MazeAlgorithm::Kruskal, 20, 20, 3);
  Random first_rng(9);
  Random second_rng(9);
  braid(first, first_rng, 0.4);
  braid(second, second_rng, 0.4);
  EXPECT_EQ(walls_of(first), walls_of(second));
}
