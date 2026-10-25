#include <gtest/gtest.h>

#include <sstream>
#include <stdexcept>
#include <string>

#include "maze_gen/export.hpp"
#include "maze_gen/maze_grid.hpp"

namespace
{

// Known 2x2 maze, (x, y) grows right and down:
//
//   +---+---+
//   | A   B |     A = (0,0)  B = (1,0)
//   +   +   +     C = (0,1)  D = (1,1)
//   | C | D |
//   +   +---+     the outer wall is open under C: that is the exit
//
// Path: A -> B -> D, and D -> C is open as well; the wall between A and C stays.
maze_gen::MazeGrid known_maze()
{
  maze_gen::MazeGrid grid(2, 2);
  grid.carve(0, 0, maze_gen::Dir::E);
  grid.carve(1, 0, maze_gen::Dir::S);
  grid.carve(1, 1, maze_gen::Dir::W);
  grid.add_exit(0, 1, maze_gen::Dir::S);
  return grid;
}

struct Exported
{
  std::string sdf, pgm, meta;
};

Exported export_known_maze()
{
  std::ostringstream sdf, pgm, meta;
  maze_gen::MazeExporter::Outputs out;
  out.sdf = &sdf;
  out.pgm = &pgm;
  out.meta = &meta;
  maze_gen::MazeExporter exporter(known_maze());
  exporter.export_maze(out);
  return {sdf.str(), pgm.str(), meta.str()};
}

size_t count(const std::string & text, const std::string & what)
{
  size_t n = 0;
  for (size_t pos = text.find(what); pos != std::string::npos; pos = text.find(what, pos + 1)) {
    ++n;
  }
  return n;
}

// Pixel of the PGM as the file stores it: the first row of the file is the bottom of the maze
int pgm_pixel(const std::string & pgm, int width, int height, int x, int row_from_bottom)
{
  const std::string header = "P5\n" + std::to_string(width) + " " + std::to_string(height) +
    "\n255\n";
  EXPECT_EQ(pgm.compare(0, header.size(), header), 0);
  return static_cast<unsigned char>(pgm[header.size() + row_from_bottom * width + x]);
}

}  // namespace

TEST(MazeExporter, KnownMazeProducesExactlyTheExpectedWalls)
{
  const auto sdf = export_known_maze().sdf;

  // One box per merged wall, each one with a collision and a visual
  EXPECT_EQ(count(sdf, "<collision name=\"w"), 5u);
  EXPECT_EQ(count(sdf, "<visual name=\"w"), 5u);
  // And a ground plane under the maze, so the robot does not fall through
  EXPECT_NE(sdf.find("<collision name=\"ground_collision\"><geometry><plane>"),
    std::string::npos);

  // Top and right sides, and the left side, in one piece each
  EXPECT_NE(sdf.find("<pose>1 0 0.25 0 0 0</pose><geometry><box><size>2.1 0.1 0.5"),
    std::string::npos);
  EXPECT_NE(sdf.find("<pose>2 1 0.25 0 0 1.5708"), std::string::npos);
  EXPECT_NE(sdf.find("<pose>0 1 0.25 0 0 1.5708"), std::string::npos);
  // The wall that separates A from C
  EXPECT_NE(sdf.find("<pose>0.5 1 0.25 0 0 0</pose><geometry><box><size>1.1 0.1 0.5"),
    std::string::npos);
  // Bottom side: only under D, since C is the exit
  EXPECT_NE(sdf.find("<pose>1.5 2 0.25 0 0 0</pose><geometry><box><size>1.1 0.1 0.5"),
    std::string::npos);
  EXPECT_EQ(sdf.find("<pose>0.5 2 "), std::string::npos);
}

TEST(MazeExporter, KnownMazeExitIsOpenInTheOccupancyMap)
{
  const auto pgm = export_known_maze().pgm;
  const int w = 40, h = 40;

  // Bottom row of the image: wall under D, free under C (the exit)
  EXPECT_EQ(pgm_pixel(pgm, w, h, 30, 0), 0);
  EXPECT_EQ(pgm_pixel(pgm, w, h, 10, 0), 255);
  // The same wall is solid on the other sides of the maze
  EXPECT_EQ(pgm_pixel(pgm, w, h, 10, h - 1), 0);  // top
  EXPECT_EQ(pgm_pixel(pgm, w, h, 39, 20), 0);     // right
  // Inside the cells there is free space
  EXPECT_EQ(pgm_pixel(pgm, w, h, 10, 10), 255);
}

TEST(MazeExporter, KnownMazeMetaPointsToTheExit)
{
  const auto meta = export_known_maze().meta;

  EXPECT_NE(meta.find("start: [0.5, 0.5]"), std::string::npos);
  EXPECT_NE(meta.find("exit: [0.5, 1.5]"), std::string::npos);
}

TEST(MazeExporter, KnownMazeMetaDescribesTheMaze)
{
  const auto meta = export_known_maze().meta;

  EXPECT_NE(meta.find("width: 2"), std::string::npos);
  EXPECT_NE(meta.find("height: 2"), std::string::npos);
  EXPECT_NE(meta.find("wall_thickness: 0.1"), std::string::npos);
  EXPECT_NE(meta.find("bounds: [0, 0, 2, 2]"), std::string::npos);
  // Loaded, not generated: there is no seed or difficulty to report
  EXPECT_EQ(meta.find("seed:"), std::string::npos);
  EXPECT_EQ(meta.find("difficulty:"), std::string::npos);
}

TEST(MazeExporter, KnownMazeMetaHasTheShortestPathToTheExit)
{
  const auto meta = export_known_maze().meta;

  // A -> B -> D -> C is 3 cells, plus half a cell from the centre of C to the gap
  EXPECT_NE(meta.find("  - {cell: [0, 1], side: S, door: [0.5, 2], distance: 3.5}"),
    std::string::npos);
  EXPECT_NE(meta.find("shortest_path: 3.5"), std::string::npos);
}

TEST(MazeExporter, StartCellSetsStartAndDistances)
{
  std::ostringstream meta;
  maze_gen::MazeExporter::Outputs out;
  out.meta = &meta;
  maze_gen::MazeExporter exporter(known_maze());
  exporter.set_start(1, 1);  // D
  exporter.export_maze(out);

  EXPECT_NE(meta.str().find("start: [1.5, 1.5]"), std::string::npos);
  // D -> C is 1 cell, plus half a cell to the gap
  EXPECT_NE(meta.str().find("  - {cell: [0, 1], side: S, door: [0.5, 2], distance: 1.5}"),
    std::string::npos);
  EXPECT_NE(meta.str().find("shortest_path: 1.5"), std::string::npos);
}

TEST(MazeExporter, StartOutsideTheGridIsRejected)
{
  maze_gen::MazeExporter exporter(known_maze());

  EXPECT_THROW(exporter.set_start(2, 0), std::out_of_range);
  EXPECT_THROW(exporter.set_start(0, 2), std::out_of_range);
  EXPECT_THROW(exporter.set_start(-1, 0), std::out_of_range);
  EXPECT_THROW(exporter.set_start(0, -1), std::out_of_range);
}

TEST(MazeExporter, UnreachableExitHasNoDistance)
{
  // Two cells with the wall between them still up: the exit of B can't be reached from A
  maze_gen::MazeGrid grid(2, 1);
  grid.add_exit(1, 0, maze_gen::Dir::E);
  std::ostringstream meta;
  maze_gen::MazeExporter::Outputs out;
  out.meta = &meta;
  maze_gen::MazeExporter(std::move(grid)).export_maze(out);

  EXPECT_NE(meta.str().find("  - {cell: [1, 0], side: E, door: [2, 0.5], distance: null}"),
    std::string::npos);
  EXPECT_NE(meta.str().find("shortest_path: null"), std::string::npos);
}

TEST(MazeExporter, GeneratedMazeMetaHasSeedAndDifficulty)
{
  std::ostringstream meta;
  maze_gen::MazeExporter::Outputs out;
  out.meta = &meta;
  maze_gen::MazeExporter(5, 5, 42, 7).export_maze(out);

  EXPECT_NE(meta.str().find("seed: 42\n"), std::string::npos);
  EXPECT_NE(meta.str().find("difficulty: 7\n"), std::string::npos);
  // A generated maze is connected: its exit can always be reached
  EXPECT_EQ(meta.str().find("distance: null"), std::string::npos);
  EXPECT_EQ(meta.str().find("shortest_path: null"), std::string::npos);
}
