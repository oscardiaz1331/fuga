// MazeGrid(path): loading a maze saved as text.
//
// Format: "width height", then one line per row with one hex digit per cell (the bits of
// Dir: a set bit is a wall), then one "exit x y N|E|S|W" line per exit.
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include "maze_gen/maze_generator.hpp"
#include "maze_gen/maze_grid.hpp"
#include "maze_test_utils.hpp"

using namespace maze_gen;  // NOLINT(build/namespaces)

namespace
{

// Deletes the file when the test ends, pass or fail
class TempFile
{
public:
  explicit TempFile(const std::string & content)
  {
    const auto * info = ::testing::UnitTest::GetInstance()->current_test_info();
    path_ = std::filesystem::temp_directory_path() /
      (std::string("maze_gen_load_") + info->name() + ".maze");
    std::ofstream(path_) << content;
  }
  ~TempFile() {std::filesystem::remove(path_);}
  TempFile(const TempFile &) = delete;
  TempFile & operator=(const TempFile &) = delete;
  const std::filesystem::path & path() const {return path_;}

private:
  std::filesystem::path path_;
};

// The 2x2 maze   +---+---+
//                | A   B |      A -> B -> D -> C, the wall between A and C stays,
//                +   +   +      and the outer wall is open under C
//                | C   D |
//                +   +---+
const char * const kKnownMaze =
  "2 2\n"
  "D3\n"
  "96\n"
  "exit 0 1 S\n";

bool loads(const std::string & content)
{
  const TempFile file(content);
  try {
    MazeGrid grid(file.path());
    return true;
  } catch (const std::runtime_error &) {
    return false;
  }
}

}  // namespace

TEST(MazeGridLoad, LoadsSizeWallsAndExitsOfAKnownMaze) {
  const TempFile file(kKnownMaze);
  const MazeGrid grid(file.path());

  EXPECT_EQ(grid.width(), 2);
  EXPECT_EQ(grid.height(), 2);
  EXPECT_EQ(grid.walls(0, 0), 0xD);
  EXPECT_EQ(grid.walls(1, 0), 0x3);
  EXPECT_EQ(grid.walls(0, 1), 0x9);
  EXPECT_EQ(grid.walls(1, 1), 0x6);
  ASSERT_EQ(grid.exits().size(), 1u);
  EXPECT_EQ(grid.exits()[0], (Exit{0, 1, Dir::S}));
}

TEST(MazeGridLoad, GeneratedMazeSurvivesASaveAndLoad) {
  MazeStrategyOptions options;
  options.seed = 7;
  MazeGrid original = MazeGenerator(MazeAlgorithm::DFS).generate(23, 17, options);
  original.add_exit(0, 0, Dir::N);
  original.add_exit(22, 16, Dir::E);

  const TempFile file(maze_gen::test::serialize(original));
  const MazeGrid loaded(file.path());

  ASSERT_EQ(loaded.width(), original.width());
  ASSERT_EQ(loaded.height(), original.height());
  for (int y = 0; y < original.height(); ++y) {
    for (int x = 0; x < original.width(); ++x) {
      EXPECT_EQ(loaded.walls(x, y), original.walls(x, y)) << "cell " << x << "," << y;
    }
  }
  EXPECT_EQ(loaded.exits(), original.exits());
}

TEST(MazeGridLoad, AcceptsLowercaseDigitsAndNoExits) {
  const TempFile file("2 1\nbe\n");
  const MazeGrid grid(file.path());
  EXPECT_EQ(grid.walls(0, 0), 0xB);
  EXPECT_EQ(grid.walls(1, 0), 0xE);
  EXPECT_TRUE(grid.exits().empty());
}

TEST(MazeGridLoad, MissingFileErrorNamesThePath) {
  const auto path = std::filesystem::temp_directory_path() / "maze_gen_does_not_exist.maze";
  try {
    MazeGrid grid(path);
    FAIL() << "no exception";
  } catch (const std::runtime_error & e) {
    EXPECT_NE(std::string(e.what()).find(path.string()), std::string::npos) << e.what();
  }
}

TEST(MazeGridLoad, DirectoryIsNotAMaze) {
  EXPECT_THROW(MazeGrid{std::filesystem::temp_directory_path()}, std::runtime_error);
}

TEST(MazeGridLoad, RejectsABadSize) {
  EXPECT_FALSE(loads(""));
  EXPECT_FALSE(loads("two by two\n"));
  EXPECT_FALSE(loads("0 2\n"));
  EXPECT_FALSE(loads("-2 2\n"));
  EXPECT_FALSE(loads("99999999999 2\n"));
}

TEST(MazeGridLoad, RejectsBadRows) {
  EXPECT_FALSE(loads("2 2\nD3\n"));         // missing row
  EXPECT_FALSE(loads("2 2\nD\n96\n"));      // short row
  EXPECT_FALSE(loads("2 2\nD3\n963\n"));    // long row
  EXPECT_FALSE(loads("2 2\nD3\n9G\n"));     // not a hex digit
}

TEST(MazeGridLoad, RejectsBadExits) {
  EXPECT_FALSE(loads("2 2\nD3\n96\nexit 0 1\n"));
  EXPECT_FALSE(loads("2 2\nD3\n96\nexit 0 1 X\n"));
  EXPECT_FALSE(loads("2 2\nD3\n96\nwindow 0 1 S\n"));
  EXPECT_FALSE(loads("2 2\nD3\n96\nexit 5 1 S\n"));                // outside the grid
  EXPECT_FALSE(loads("2 2\nD3\n96\nexit 0 0 E\n"));                // points into the maze
  EXPECT_FALSE(loads("2 2\nD3\n96\nexit 0 1 S\nexit 0 1 S\n"));    // repeated
}
