// RegionConnector on small hand-made grids, where the right answer is known.
#include <gtest/gtest.h>
#include "maze_test_utils.hpp"
#include "region_connector.hpp"

using namespace maze_gen;  // NOLINT(build/namespaces)
using maze_gen::test::analyze;
using maze_gen::test::walls_of;

namespace
{

// Carve a straight corridor through the cells x0..x1 of row y
void carve_row(MazeGrid & grid, int y, int x0, int x1)
{
  for (int x = x0; x < x1; ++x) {
    grid.carve(x, y, Dir::E);
  }
}

bool has_passage(const MazeGrid & grid, int x, int y)
{
  for (const auto & [nx, ny, d] : grid.neighbors(x, y)) {
    if (!grid.has_wall(x, y, d)) {
      return true;
    }
  }
  return false;
}

}  // namespace

// ---------- nothing to do ----------

TEST(RegionConnector, DoesNothingOnAFullyWalledGrid) {
  // Sealed cells are solid rock, not regions waiting to be reached
  MazeGrid grid(4, 4);
  EXPECT_EQ(RegionConnector().connect(grid), 0);
  EXPECT_EQ(walls_of(grid), walls_of(MazeGrid(4, 4)));
}

TEST(RegionConnector, DoesNothingWithASingleRegion) {
  MazeGrid grid(4, 4);
  carve_row(grid, 1, 0, 3);
  const auto before = walls_of(grid);
  EXPECT_EQ(RegionConnector().connect(grid), 0);
  EXPECT_EQ(walls_of(grid), before);
}

TEST(RegionConnector, DoesNothingWhenExitsAreAlreadyJoined) {
  MazeGrid grid(5, 1);
  carve_row(grid, 0, 0, 4);
  grid.add_exit(0, 0, Dir::W);
  grid.add_exit(4, 0, Dir::E);
  const auto before = walls_of(grid);
  EXPECT_EQ(RegionConnector().connect(grid), 0);
  EXPECT_EQ(walls_of(grid), before);
}

TEST(RegionConnector, DoesNothingWithOneSealedExit) {
  MazeGrid grid(4, 4);
  grid.add_exit(0, 1, Dir::W);
  const auto before = walls_of(grid);
  EXPECT_EQ(RegionConnector().connect(grid), 0);
  EXPECT_EQ(walls_of(grid), before);
}

TEST(RegionConnector, WorksOnASingleCell) {
  MazeGrid grid(1, 1);
  grid.add_exit(0, 0, Dir::N);
  grid.add_exit(0, 0, Dir::S);
  EXPECT_EQ(RegionConnector().connect(grid), 0);
  EXPECT_EQ(analyze(grid).border_openings, 2);
}

// ---------- joining ----------

TEST(RegionConnector, BreaksTheSingleWallBetweenTwoCorridors) {
  MazeGrid grid(6, 1);
  carve_row(grid, 0, 0, 2);
  carve_row(grid, 0, 3, 5);
  ASSERT_EQ(analyze(grid).regions, 2);

  EXPECT_EQ(RegionConnector().connect(grid), 1);

  EXPECT_EQ(analyze(grid).regions, 1);
  EXPECT_FALSE(grid.has_wall(2, 0, Dir::E));
}

TEST(RegionConnector, PicksTheThinnestSpotBetweenTwoRegions) {
  // Two corridors side by side: a single wall is enough wherever it is broken
  MazeGrid grid(4, 2);
  carve_row(grid, 0, 0, 3);
  carve_row(grid, 1, 0, 3);
  const int before = analyze(grid).passages;

  EXPECT_EQ(RegionConnector().connect(grid), 1);

  EXPECT_EQ(analyze(grid).regions, 1);
  EXPECT_EQ(analyze(grid).passages - before, 1);
}

TEST(RegionConnector, DigsThroughRockBetweenTwoSealedExits) {
  MazeGrid grid(4, 1);  // nothing carved: three walls between the two ends
  grid.add_exit(0, 0, Dir::W);
  grid.add_exit(3, 0, Dir::E);

  EXPECT_EQ(RegionConnector().connect(grid), 3);

  const auto stats = analyze(grid);
  EXPECT_EQ(stats.regions, 1);
  EXPECT_EQ(stats.passages, 3);
}

// The straight line between the exits crosses 6 walls; going down to the existing
// corridor and back up crosses only 4
TEST(RegionConnector, PrefersExistingCorridorsOverBreakingWalls) {
  MazeGrid grid(7, 3);
  carve_row(grid, 2, 0, 6);  // corridor along the bottom row
  grid.add_exit(0, 0, Dir::N);
  grid.add_exit(6, 0, Dir::N);

  EXPECT_EQ(RegionConnector().connect(grid), 4);

  EXPECT_EQ(analyze(grid).regions, 1);
  for (int x = 0; x < 6; ++x) {
    EXPECT_TRUE(grid.has_wall(x, 0, Dir::E)) << "the top row was tunnelled at x=" << x;
  }
}

TEST(RegionConnector, JoinsManyIslandsInOneCall) {
  // Ten corridors of four cells that fill the grid, each one wall away from the next
  MazeGrid grid(8, 5);
  for (int y = 0; y < grid.height(); ++y) {
    carve_row(grid, y, 0, 3);
    carve_row(grid, y, 4, 7);
  }
  ASSERT_EQ(analyze(grid).regions, 10);

  // Ten regions need nine bridges, and every bridge here is a single wall
  EXPECT_EQ(RegionConnector().connect(grid), 9);

  const auto stats = analyze(grid);
  EXPECT_EQ(stats.regions, 1);
  EXPECT_EQ(stats.asymmetric_walls, 0);
  EXPECT_EQ(stats.border_openings, 0);
}

TEST(RegionConnector, JoinsIslandsAndExitsTogether) {
  MazeGrid grid(6, 5);
  carve_row(grid, 0, 0, 2);
  carve_row(grid, 4, 3, 5);
  grid.add_exit(5, 2, Dir::E);  // sealed cell, away from both islands
  ASSERT_EQ(analyze(grid).regions, 2);

  RegionConnector().connect(grid);

  EXPECT_EQ(analyze(grid).regions, 1);
  EXPECT_TRUE(has_passage(grid, 5, 2));
}

TEST(RegionConnector, JoinsEveryExitNotJustTheFirstTwo) {
  MazeGrid grid(7, 7);
  grid.add_exit(0, 3, Dir::W);
  grid.add_exit(6, 3, Dir::E);
  grid.add_exit(3, 0, Dir::N);
  grid.add_exit(3, 6, Dir::S);

  RegionConnector().connect(grid);

  EXPECT_EQ(analyze(grid).regions, 1);
  for (const Exit & exit : grid.exits()) {
    EXPECT_TRUE(has_passage(grid, exit.x, exit.y)) << ::testing::PrintToString(exit);
  }
}

// ---------- what it must not do ----------

TEST(RegionConnector, LeavesSolidRockAwayFromThePathAlone) {
  MazeGrid grid(6, 6);
  carve_row(grid, 0, 0, 1);
  carve_row(grid, 0, 3, 4);
  RegionConnector().connect(grid);
  // The bridge runs along the top row: the rows below stay sealed
  for (int y = 2; y < grid.height(); ++y) {
    for (int x = 0; x < grid.width(); ++x) {
      EXPECT_EQ(grid.walls(x, y), kAllWalls) << "cell " << x << "," << y;
    }
  }
}

TEST(RegionConnector, NeverOpensTheBorder) {
  MazeGrid grid(6, 6);
  grid.add_exit(0, 2, Dir::W);
  grid.add_exit(5, 4, Dir::E);
  RegionConnector().connect(grid);
  EXPECT_EQ(analyze(grid).border_openings, 2);
  EXPECT_EQ(analyze(grid).asymmetric_walls, 0);
}

TEST(RegionConnector, RunningItTwiceChangesNothingMore) {
  MazeGrid grid(8, 8);
  carve_row(grid, 0, 0, 3);
  carve_row(grid, 7, 4, 7);
  grid.add_exit(0, 4, Dir::W);
  RegionConnector connector;
  EXPECT_GT(connector.connect(grid), 0);
  const auto once = walls_of(grid);

  EXPECT_EQ(connector.connect(grid), 0);
  EXPECT_EQ(walls_of(grid), once);
}

TEST(RegionConnector, ReportsExactlyTheWallsItBroke) {
  MazeGrid grid(10, 10);
  carve_row(grid, 1, 0, 2);
  carve_row(grid, 5, 6, 9);
  carve_row(grid, 8, 1, 3);
  grid.add_exit(9, 9, Dir::S);
  const int before = analyze(grid).passages;

  const int broken = RegionConnector().connect(grid);

  EXPECT_EQ(analyze(grid).passages - before, broken);
}
