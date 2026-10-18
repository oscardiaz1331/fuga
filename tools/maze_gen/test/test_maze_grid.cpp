// MazeGrid: construction, wall operations, neighbors, exits and reset.
#include <gtest/gtest.h>
#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <string>
#include "maze_gen/maze_grid.hpp"
#include "maze_test_utils.hpp"

using namespace maze_gen;  // NOLINT(build/namespaces)
using maze_gen::test::analyze;
using maze_gen::test::walls_of;

// ---------- construction ----------

TEST(MazeGridConstruction, KeepsTheRequestedSize) {
  MazeGrid grid(7, 3);
  EXPECT_EQ(grid.width(), 7);
  EXPECT_EQ(grid.height(), 3);
  EXPECT_EQ(grid.cell_count(), 21);
}

TEST(MazeGridConstruction, SingleCellIsValid) {
  MazeGrid grid(1, 1);
  EXPECT_EQ(grid.cell_count(), 1);
  EXPECT_EQ(grid.walls(0, 0), kAllWalls);
}

TEST(MazeGridConstruction, StartsWithEveryWallAndNoExits) {
  MazeGrid grid(4, 5);
  for (int y = 0; y < grid.height(); ++y) {
    for (int x = 0; x < grid.width(); ++x) {
      EXPECT_EQ(grid.walls(x, y), kAllWalls) << "cell " << x << "," << y;
    }
  }
  EXPECT_TRUE(grid.exits().empty());

  const auto stats = analyze(grid);
  EXPECT_EQ(stats.closed_cells, 20);
  EXPECT_EQ(stats.passages, 0);
  EXPECT_EQ(stats.border_openings, 0);
}

TEST(MazeGridConstruction, RejectsZeroSize) {
  EXPECT_THROW(MazeGrid(0, 0), std::invalid_argument);
  EXPECT_THROW(MazeGrid(0, 5), std::invalid_argument);
  EXPECT_THROW(MazeGrid(5, 0), std::invalid_argument);
}

TEST(MazeGridConstruction, RejectsNegativeSize) {
  EXPECT_THROW(MazeGrid(-1, 5), std::invalid_argument);
  EXPECT_THROW(MazeGrid(5, -1), std::invalid_argument);
  EXPECT_THROW(MazeGrid(-3, -3), std::invalid_argument);
}

TEST(MazeGridConstruction, RejectsSizesWhoseCellCountOverflowsInt) {
  EXPECT_THROW(MazeGrid(70000, 70000), std::invalid_argument);
  EXPECT_THROW(MazeGrid(2147483647, 2), std::invalid_argument);
}

// ---------- geometry ----------

TEST(MazeGridGeometry, InBoundsAcceptsOnlyCellsOfTheGrid) {
  MazeGrid grid(3, 2);
  EXPECT_TRUE(grid.in_bounds(0, 0));
  EXPECT_TRUE(grid.in_bounds(2, 1));
  EXPECT_FALSE(grid.in_bounds(-1, 0));
  EXPECT_FALSE(grid.in_bounds(0, -1));
  EXPECT_FALSE(grid.in_bounds(3, 0));
  EXPECT_FALSE(grid.in_bounds(0, 2));
}

TEST(MazeGridGeometry, CellIndexIsRowMajor) {
  MazeGrid grid(4, 3);
  EXPECT_EQ(grid.cell_index(0, 0), 0);
  EXPECT_EQ(grid.cell_index(3, 0), 3);
  EXPECT_EQ(grid.cell_index(0, 1), 4);
  EXPECT_EQ(grid.cell_index(3, 2), 11);
}

TEST(MazeGridGeometry, NorthIsUpAndEastIsRight) {
  EXPECT_EQ(MazeGrid::neighbor(5, 5, Dir::N), std::make_pair(5, 4));
  EXPECT_EQ(MazeGrid::neighbor(5, 5, Dir::S), std::make_pair(5, 6));
  EXPECT_EQ(MazeGrid::neighbor(5, 5, Dir::E), std::make_pair(6, 5));
  EXPECT_EQ(MazeGrid::neighbor(5, 5, Dir::W), std::make_pair(4, 5));
}

TEST(MazeGridGeometry, OppositeIsAnInvolution) {
  for (Dir d : kDirections) {
    EXPECT_NE(opposite(d), d);
    EXPECT_EQ(opposite(opposite(d)), d);
  }
  EXPECT_EQ(opposite(Dir::N), Dir::S);
  EXPECT_EQ(opposite(Dir::E), Dir::W);
}

TEST(MazeGridGeometry, EachDirectionHasItsOwnWallBit) {
  uint8_t all = 0;
  for (Dir d : kDirections) {
    const uint8_t bit = static_cast<uint8_t>(d);
    EXPECT_EQ(bit & (bit - 1), 0) << "not a single bit";
    EXPECT_EQ(all & bit, 0) << "bit used twice";
    all |= bit;
  }
  EXPECT_EQ(all, kAllWalls);
}

// ---------- neighbors ----------

TEST(MazeGridNeighbors, CountDependsOnThePosition) {
  MazeGrid grid(3, 3);
  EXPECT_EQ(grid.neighbors(1, 1).size(), 4u);  // center
  EXPECT_EQ(grid.neighbors(1, 0).size(), 3u);  // edge
  EXPECT_EQ(grid.neighbors(0, 0).size(), 2u);  // corner
}

TEST(MazeGridNeighbors, SingleCellHasNone) {
  MazeGrid grid(1, 1);
  EXPECT_TRUE(grid.neighbors(0, 0).empty());
}

TEST(MazeGridNeighbors, EveryNeighborIsInsideAndMatchesItsDirection) {
  MazeGrid grid(4, 3);
  for (int y = 0; y < grid.height(); ++y) {
    for (int x = 0; x < grid.width(); ++x) {
      for (const auto & [nx, ny, dir] : grid.neighbors(x, y)) {
        EXPECT_TRUE(grid.in_bounds(nx, ny));
        EXPECT_EQ(std::make_pair(nx, ny), MazeGrid::neighbor(x, y, dir));
      }
    }
  }
}

TEST(MazeGridNeighbors, ComeInNorthEastSouthWestOrder) {
  // The algorithms draw random positions in this list, so its order is part of what
  // makes a seed reproducible
  MazeGrid grid(3, 3);
  const auto neighbors = grid.neighbors(1, 1);
  ASSERT_EQ(neighbors.size(), 4u);
  EXPECT_EQ(neighbors[0].dir, Dir::N);
  EXPECT_EQ(neighbors[1].dir, Dir::E);
  EXPECT_EQ(neighbors[2].dir, Dir::S);
  EXPECT_EQ(neighbors[3].dir, Dir::W);
}

// ---------- carve ----------

TEST(MazeGridCarve, OpensTheWallOnBothSides) {
  MazeGrid grid(2, 1);
  EXPECT_TRUE(grid.carve(0, 0, Dir::E));
  EXPECT_FALSE(grid.has_wall(0, 0, Dir::E));
  EXPECT_FALSE(grid.has_wall(1, 0, Dir::W));

  const auto stats = analyze(grid);
  EXPECT_EQ(stats.passages, 1);
  EXPECT_EQ(stats.asymmetric_walls, 0);
}

TEST(MazeGridCarve, LeavesTheOtherWallsAlone) {
  MazeGrid grid(3, 3);
  grid.carve(1, 1, Dir::N);
  EXPECT_TRUE(grid.has_wall(1, 1, Dir::E));
  EXPECT_TRUE(grid.has_wall(1, 1, Dir::S));
  EXPECT_TRUE(grid.has_wall(1, 1, Dir::W));
  EXPECT_EQ(grid.walls(0, 0), kAllWalls);
  EXPECT_EQ(grid.walls(2, 2), kAllWalls);
}

TEST(MazeGridCarve, IsIdempotent) {
  MazeGrid grid(2, 1);
  grid.carve(0, 0, Dir::E);
  const auto once = walls_of(grid);
  EXPECT_TRUE(grid.carve(0, 0, Dir::E));
  EXPECT_TRUE(grid.carve(1, 0, Dir::W));
  EXPECT_EQ(walls_of(grid), once);
}

TEST(MazeGridCarve, NeverOpensTheOuterWall) {
  MazeGrid grid(2, 2);
  EXPECT_FALSE(grid.carve(0, 0, Dir::N));
  EXPECT_FALSE(grid.carve(0, 0, Dir::W));
  EXPECT_FALSE(grid.carve(1, 1, Dir::S));
  EXPECT_FALSE(grid.carve(1, 1, Dir::E));
  EXPECT_EQ(walls_of(grid), walls_of(MazeGrid(2, 2)));
}

TEST(MazeGridCarve, IgnoresCellsOutsideTheGrid) {
  MazeGrid grid(2, 2);
  EXPECT_FALSE(grid.carve(-1, 0, Dir::E));
  EXPECT_FALSE(grid.carve(0, -1, Dir::S));
  EXPECT_FALSE(grid.carve(2, 0, Dir::W));
  EXPECT_FALSE(grid.carve(0, 2, Dir::N));
  EXPECT_FALSE(grid.carve(100, 100, Dir::N));
  EXPECT_EQ(walls_of(grid), walls_of(MazeGrid(2, 2)));
}

// ---------- add_wall ----------

TEST(MazeGridAddWall, ClosesAPassageOnBothSides) {
  MazeGrid grid(2, 1);
  grid.carve(0, 0, Dir::E);
  EXPECT_TRUE(grid.add_wall(1, 0, Dir::W));
  EXPECT_TRUE(grid.has_wall(0, 0, Dir::E));
  EXPECT_TRUE(grid.has_wall(1, 0, Dir::W));
  EXPECT_EQ(analyze(grid).passages, 0);
}

TEST(MazeGridAddWall, UndoesCarveExactly) {
  MazeGrid grid(3, 3);
  const auto untouched = walls_of(grid);
  for (Dir d : kDirections) {
    grid.carve(1, 1, d);
  }
  EXPECT_NE(walls_of(grid), untouched);
  for (Dir d : kDirections) {
    grid.add_wall(1, 1, d);
  }
  EXPECT_EQ(walls_of(grid), untouched);
}

TEST(MazeGridAddWall, RejectsCellsAndNeighborsOutsideTheGrid) {
  MazeGrid grid(2, 2);
  EXPECT_FALSE(grid.add_wall(0, 0, Dir::N));
  EXPECT_FALSE(grid.add_wall(-1, 0, Dir::E));
  EXPECT_FALSE(grid.add_wall(5, 5, Dir::W));
}

// ---------- exits ----------

TEST(MazeGridExits, AddExitOpensTheOuterWallAndRecordsIt) {
  MazeGrid grid(3, 3);
  EXPECT_TRUE(grid.add_exit(1, 0, Dir::N));
  EXPECT_FALSE(grid.has_wall(1, 0, Dir::N));
  ASSERT_EQ(grid.exits().size(), 1u);
  EXPECT_EQ(grid.exits()[0], (Exit{1, 0, Dir::N}));

  const auto stats = analyze(grid);
  EXPECT_EQ(stats.border_openings, 1);
  EXPECT_EQ(stats.passages, 0);
  EXPECT_EQ(grid.walls(1, 1), kAllWalls);
}

TEST(MazeGridExits, AreKeptInInsertionOrder) {
  MazeGrid grid(3, 3);
  grid.add_exit(0, 1, Dir::W);
  grid.add_exit(2, 1, Dir::E);
  const std::vector<Exit> expected = {{0, 1, Dir::W}, {2, 1, Dir::E}};
  EXPECT_EQ(grid.exits(), expected);
}

TEST(MazeGridExits, ACornerCellCanHaveOneOnEachOuterSide) {
  MazeGrid grid(3, 3);
  EXPECT_TRUE(grid.add_exit(0, 0, Dir::N));
  EXPECT_TRUE(grid.add_exit(0, 0, Dir::W));
  EXPECT_EQ(grid.exits().size(), 2u);
  EXPECT_EQ(analyze(grid).border_openings, 2);
}

// An exit that was not opened must never be recorded: later code uses it as a cell
TEST(MazeGridExits, RejectsDirectionsThatStayInside) {
  MazeGrid grid(3, 3);
  EXPECT_FALSE(grid.add_exit(1, 0, Dir::S));
  EXPECT_FALSE(grid.add_exit(1, 0, Dir::E));
  EXPECT_FALSE(grid.add_exit(1, 1, Dir::N));
  EXPECT_TRUE(grid.exits().empty());
  EXPECT_EQ(walls_of(grid), walls_of(MazeGrid(3, 3)));
}

TEST(MazeGridExits, RejectsCellsOutsideTheGrid) {
  MazeGrid grid(3, 3);
  EXPECT_FALSE(grid.add_exit(-1, 0, Dir::N));
  EXPECT_FALSE(grid.add_exit(3, 0, Dir::N));
  EXPECT_FALSE(grid.add_exit(0, 3, Dir::S));
  EXPECT_FALSE(grid.add_exit(500, 500, Dir::E));
  EXPECT_TRUE(grid.exits().empty());
  EXPECT_EQ(analyze(grid).border_openings, 0);
}

TEST(MazeGridExits, RejectsARepeatedExit) {
  MazeGrid grid(3, 3);
  EXPECT_TRUE(grid.add_exit(0, 1, Dir::W));
  EXPECT_FALSE(grid.add_exit(0, 1, Dir::W));
  EXPECT_EQ(grid.exits().size(), 1u);
}

// ---------- reset ----------

TEST(MazeGridReset, PutsEveryWallBack) {
  MazeGrid grid(3, 3);
  grid.carve(0, 0, Dir::E);
  grid.carve(1, 1, Dir::S);
  grid.add_exit(0, 0, Dir::N);

  grid.reset();

  EXPECT_EQ(walls_of(grid), walls_of(MazeGrid(3, 3)));
}

TEST(MazeGridReset, ForgetsTheExits) {
  MazeGrid grid(3, 3);
  grid.add_exit(0, 1, Dir::W);
  grid.reset();
  EXPECT_TRUE(grid.exits().empty());
  EXPECT_TRUE(grid.add_exit(0, 1, Dir::W)) << "the same exit can be added again after a reset";
}

// ---------- display ----------

TEST(MazeGridDisplay, SimpleDrawsWallsAndPassages) {
  MazeGrid grid(2, 1);
  grid.carve(0, 0, Dir::E);
  std::ostringstream out;
  grid.display_simple(out);
  EXPECT_EQ(
    out.str(),
    "+---+---+\n"
    "|       |\n"
    "+---+---+\n");
}

TEST(MazeGridDisplay, SimpleShowsAnExit) {
  MazeGrid grid(1, 1);
  grid.add_exit(0, 0, Dir::N);
  std::ostringstream out;
  grid.display_simple(out);
  EXPECT_EQ(
    out.str(),
    "+   +\n"
    "|   |\n"
    "+---+\n");
}

TEST(MazeGridDisplay, ComplexPrintsOneSymbolPerCell) {
  MazeGrid grid(2, 1);
  grid.carve(0, 0, Dir::E);
  std::ostringstream out;
  grid.display_complex(out);
  // West cell: walls north, south and west. East cell: north, south and east.
  EXPECT_EQ(out.str(), "┤├\n");
}

TEST(MazeGridDisplay, ComplexPrintsOneLinePerRow) {
  MazeGrid grid(3, 4);
  std::ostringstream out;
  grid.display_complex(out);
  const std::string text = out.str();
  EXPECT_EQ(std::count(text.begin(), text.end(), '\n'), 4);
}
