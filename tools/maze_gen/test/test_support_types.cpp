// The small building blocks: Random, the algorithm names, Room, UnionFind and the
// box-drawing table.
#include <gtest/gtest.h>
#include <algorithm>
#include <numeric>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include "box_drawing_chars.hpp"
#include "maze_test_utils.hpp"
#include "room.hpp"
#include "union_find.hpp"

using namespace maze_gen;  // NOLINT(build/namespaces)

namespace
{

using CellSet = std::set<std::pair<int, int>>;

}  // namespace

// ===================== Random =====================

TEST(Random, SameSeedGivesTheSameSequence) {
  Random first(1234);
  Random second(1234);
  for (int i = 0; i < 100; ++i) {
    EXPECT_EQ(first.next(), second.next());
  }
}

TEST(Random, DifferentSeedsGiveDifferentSequences) {
  Random first(1);
  Random second(2);
  bool differs = false;
  for (int i = 0; i < 10; ++i) {
    differs = differs || first.next() != second.next();
  }
  EXPECT_TRUE(differs);
}

// A 64-bit seed is split into two halves: both of them have to matter
TEST(Random, UsesAllSixtyFourBitsOfTheSeed) {
  Random low(0x0000000000000001ull);
  Random high(0x0000000100000001ull);
  bool differs = false;
  for (int i = 0; i < 10; ++i) {
    differs = differs || low.next() != high.next();
  }
  EXPECT_TRUE(differs);
}

// The phases of one generation share the seed and are told apart by the stream
TEST(Random, DifferentStreamsGiveDifferentSequences) {
  Random first(42, 0);
  Random second(42, 1);
  bool differs = false;
  for (int i = 0; i < 10; ++i) {
    differs = differs || first.next() != second.next();
  }
  EXPECT_TRUE(differs);
}

TEST(Random, WithoutASeedEveryGeneratorIsDifferent) {
  Random first(std::nullopt);
  Random second(std::nullopt);
  bool differs = false;
  for (int i = 0; i < 10; ++i) {
    differs = differs || first.next() != second.next();
  }
  EXPECT_TRUE(differs);
}

TEST(Random, AcceptsTheExtremeSeeds) {
  EXPECT_NO_THROW(Random(0));
  EXPECT_NO_THROW(Random(0xFFFFFFFFFFFFFFFFull));
}

TEST(Random, BelowStaysInRange) {
  Random rng(1);
  for (uint32_t n : {1u, 2u, 3u, 7u, 100u, 0x80000001u, 0xFFFFFFFFu}) {
    for (int i = 0; i < 200; ++i) {
      EXPECT_LT(rng.below(n), n);
    }
  }
}

TEST(Random, BelowOneIsAlwaysZero) {
  Random rng(1);
  for (int i = 0; i < 50; ++i) {
    EXPECT_EQ(rng.below(1), 0u);
  }
}

TEST(Random, UniformIntCoversTheWholeRangeAndNothingElse) {
  Random rng(1);
  std::set<int> seen;
  for (int i = 0; i < 2000; ++i) {
    const int value = rng.uniform_int(-3, 4);
    EXPECT_GE(value, -3);
    EXPECT_LE(value, 4);
    seen.insert(value);
  }
  EXPECT_EQ(seen.size(), 8u);
}

TEST(Random, UniformIntWithASingleValue) {
  Random rng(1);
  for (int i = 0; i < 50; ++i) {
    EXPECT_EQ(rng.uniform_int(5, 5), 5);
  }
}

TEST(Random, UniformIntIsRoughlyUniform) {
  Random rng(7);
  int counts[6] = {};
  const int draws = 60000;
  for (int i = 0; i < draws; ++i) {
    ++counts[rng.uniform_int(0, 5)];
  }
  for (int count : counts) {
    EXPECT_NEAR(count, draws / 6, draws / 6 * 0.05);
  }
}

TEST(Random, UnitStaysBelowOne) {
  Random rng(1);
  double sum = 0;
  const int draws = 20000;
  for (int i = 0; i < draws; ++i) {
    const double value = rng.unit();
    EXPECT_GE(value, 0.0);
    EXPECT_LT(value, 1.0);
    sum += value;
  }
  EXPECT_NEAR(sum / draws, 0.5, 0.01);
}

TEST(Random, ChanceIsNeverTrueAtZeroAndAlwaysTrueAtOne) {
  Random rng(1);
  for (int i = 0; i < 1000; ++i) {
    EXPECT_FALSE(rng.chance(0.0));
    EXPECT_TRUE(rng.chance(1.0));
  }
}

TEST(Random, ChanceFollowsItsProbability) {
  Random rng(3);
  int hits = 0;
  const int draws = 40000;
  for (int i = 0; i < draws; ++i) {
    hits += rng.chance(0.25) ? 1 : 0;
  }
  EXPECT_NEAR(hits, draws * 0.25, draws * 0.01);
}

TEST(Random, ShuffleKeepsEveryElement) {
  Random rng(1);
  std::vector<int> values(50);
  std::iota(values.begin(), values.end(), 0);
  const std::vector<int> original = values;

  rng.shuffle(values.begin(), values.end());

  EXPECT_NE(values, original);
  std::sort(values.begin(), values.end());
  EXPECT_EQ(values, original);
}

TEST(Random, ShuffleHandlesEmptyAndSingleElementRanges) {
  Random rng(1);
  std::vector<int> empty;
  rng.shuffle(empty.begin(), empty.end());
  EXPECT_TRUE(empty.empty());

  std::vector<int> single = {7};
  rng.shuffle(single.begin(), single.end());
  EXPECT_EQ(single, std::vector<int>{7});
}

TEST(Random, ShufflePutsEveryElementEverywhere) {
  // Each of the 3 values has to show up in each of the 3 positions about a third of the time
  Random rng(5);
  int counts[3][3] = {};
  const int rounds = 30000;
  for (int i = 0; i < rounds; ++i) {
    int values[3] = {0, 1, 2};
    rng.shuffle(values, values + 3);
    for (int position = 0; position < 3; ++position) {
      ++counts[values[position]][position];
    }
  }
  for (const auto & row : counts) {
    for (int count : row) {
      EXPECT_NEAR(count, rounds / 3, rounds / 3 * 0.05);
    }
  }
}

// ===================== algorithm names =====================

TEST(AlgorithmNames, EveryAlgorithmHasALowerCaseName) {
  for (MazeAlgorithm algorithm : all_algorithms()) {
    const std::string name(to_string(algorithm));
    EXPECT_FALSE(name.empty());
    for (char c : name) {
      EXPECT_TRUE((c >= 'a' && c <= 'z') || c == '_') << name;
    }
  }
}

TEST(AlgorithmNames, AreAllDifferent) {
  std::set<std::string> names;
  for (MazeAlgorithm algorithm : all_algorithms()) {
    names.insert(std::string(to_string(algorithm)));
  }
  EXPECT_EQ(names.size(), all_algorithms().size());
}

TEST(AlgorithmNames, RoundTripThroughText) {
  for (MazeAlgorithm algorithm : all_algorithms()) {
    const auto parsed = algorithm_from_string(to_string(algorithm));
    ASSERT_TRUE(parsed.has_value()) << to_string(algorithm);
    EXPECT_EQ(*parsed, algorithm);
  }
}

TEST(AlgorithmNames, KnownNames) {
  EXPECT_EQ(to_string(MazeAlgorithm::DFS), "dfs");
  EXPECT_EQ(to_string(MazeAlgorithm::HuntAndKill), "hunt_and_kill");
  EXPECT_EQ(to_string(MazeAlgorithm::CellularAutomaton), "cellular_automaton");
  EXPECT_EQ(algorithm_from_string("wilson"), MazeAlgorithm::Wilson);
  EXPECT_EQ(algorithm_from_string("recursive_division"), MazeAlgorithm::RecursiveDivision);
}

TEST(AlgorithmNames, UnknownTextIsNotAnAlgorithm) {
  for (const char * name : {"", " ", "DFS", "Dfs", "dfs ", " dfs", "d", "hunt-and-kill",
      "hunt and kill", "nope", "dfs\n"})
  {
    EXPECT_FALSE(algorithm_from_string(name).has_value()) << "'" << name << "'";
  }
}

TEST(AlgorithmNames, AnUnknownValueHasNoName) {
  EXPECT_THROW(to_string(static_cast<MazeAlgorithm>(9999)), std::invalid_argument);
  EXPECT_THROW(to_string(static_cast<MazeAlgorithm>(-1)), std::invalid_argument);
}

TEST(AlgorithmList, HasThirteenAlgorithmsEachOnce) {
  const auto all = all_algorithms();
  EXPECT_EQ(all.size(), 13u);
  EXPECT_EQ(std::set<MazeAlgorithm>(all.begin(), all.end()).size(), 13u);
}

TEST(AlgorithmList, OnlyTheCaveGeneratorIsNotPerfect) {
  for (MazeAlgorithm algorithm : all_algorithms()) {
    EXPECT_EQ(algorithm_info(algorithm).perfect, algorithm != MazeAlgorithm::CellularAutomaton)
      << to_string(algorithm);
  }
}

TEST(AlgorithmList, EveryAlgorithmCanBeCreatedAndGivesANewObjectEachTime) {
  for (MazeAlgorithm algorithm : all_algorithms()) {
    auto first = algorithm_info(algorithm).create();
    auto second = algorithm_info(algorithm).create();
    ASSERT_NE(first, nullptr) << to_string(algorithm);
    EXPECT_NE(first.get(), second.get()) << to_string(algorithm);
  }
}

// ===================== Room =====================

TEST(Room, ReportsItsGeometry) {
  const Room room(2, 3, 5, 4);
  EXPECT_EQ(room.top_left(), std::make_pair(2, 3));
  EXPECT_EQ(room.width(), 5);
  EXPECT_EQ(room.height(), 4);
}

TEST(Room, ContainsItsCellsAndNothingElse) {
  const Room room(2, 3, 5, 4);  // x 2..6, y 3..6
  EXPECT_TRUE(room.contains(2, 3));
  EXPECT_TRUE(room.contains(6, 6));
  EXPECT_TRUE(room.contains(4, 5));
  EXPECT_FALSE(room.contains(1, 3));
  EXPECT_FALSE(room.contains(2, 2));
  EXPECT_FALSE(room.contains(7, 6));
  EXPECT_FALSE(room.contains(6, 7));
}

TEST(Room, OverlapsWhenTheyShareCells) {
  const Room room(0, 0, 3, 3);
  EXPECT_TRUE(room.overlaps(Room(2, 2, 3, 3)));
  EXPECT_TRUE(room.overlaps(Room(1, 1, 1, 1)));
  EXPECT_TRUE(room.overlaps(room));
}

TEST(Room, TouchingRoomsOverlapBecauseOfTheBuffer) {
  const Room room(0, 0, 3, 3);
  EXPECT_TRUE(room.overlaps(Room(3, 0, 3, 3)));   // side by side
  EXPECT_TRUE(room.overlaps(Room(0, 3, 3, 3)));   // one above the other
  EXPECT_FALSE(room.overlaps(Room(3, 0, 3, 3), 0));
}

TEST(Room, DoNotOverlapWithACellOfGap) {
  const Room room(0, 0, 3, 3);
  EXPECT_FALSE(room.overlaps(Room(4, 0, 3, 3)));
  EXPECT_FALSE(room.overlaps(Room(0, 4, 3, 3)));
  EXPECT_TRUE(room.overlaps(Room(4, 0, 3, 3), 2));  // a wider buffer reaches it
}

TEST(Room, OverlapIsSymmetric) {
  const Room a(0, 0, 4, 2);
  const Room b(3, 1, 2, 5);
  const Room c(10, 10, 2, 2);
  EXPECT_EQ(a.overlaps(b), b.overlaps(a));
  EXPECT_EQ(a.overlaps(c), c.overlaps(a));
}

TEST(Room, PerimeterIsTheOuterRingWithoutRepeats) {
  const Room room(2, 3, 4, 3);
  const auto perimeter = room.perimeter_cells();
  EXPECT_EQ(perimeter.size(), 10u);  // 4 x 3 cells minus the 2 in the middle
  EXPECT_EQ(CellSet(perimeter.begin(), perimeter.end()).size(), 10u);
  for (const auto & [x, y] : perimeter) {
    EXPECT_TRUE(room.contains(x, y));
    EXPECT_TRUE(x == 2 || x == 5 || y == 3 || y == 5);
  }
}

TEST(Room, PerimeterOfThinRoomsHasNoRepeats) {
  for (const Room & room : {Room(0, 0, 2, 2), Room(0, 0, 5, 1), Room(0, 0, 1, 5),
      Room(0, 0, 1, 1)})
  {
    const auto perimeter = room.perimeter_cells();
    EXPECT_EQ(perimeter.size(), static_cast<size_t>(room.width() * room.height()));
    EXPECT_EQ(CellSet(perimeter.begin(), perimeter.end()).size(), perimeter.size());
  }
}

// ===================== UnionFind =====================

TEST(UnionFind, StartsWithEveryElementAlone) {
  UnionFind sets(5);
  for (int i = 0; i < 5; ++i) {
    EXPECT_EQ(sets.find(i), i);
    for (int j = i + 1; j < 5; ++j) {
      EXPECT_FALSE(sets.connected(i, j));
    }
  }
}

TEST(UnionFind, UniteJoinsTwoSets) {
  UnionFind sets(5);
  sets.unite(0, 1);
  EXPECT_TRUE(sets.connected(0, 1));
  EXPECT_TRUE(sets.connected(1, 0));
  EXPECT_FALSE(sets.connected(0, 2));
}

TEST(UnionFind, ConnectionIsTransitive) {
  UnionFind sets(6);
  sets.unite(0, 1);
  sets.unite(2, 3);
  EXPECT_FALSE(sets.connected(1, 2));
  sets.unite(1, 2);
  EXPECT_TRUE(sets.connected(0, 3));
  EXPECT_FALSE(sets.connected(0, 4));
  EXPECT_FALSE(sets.connected(4, 5));
}

TEST(UnionFind, UnitingTwiceOrWithItselfIsHarmless) {
  UnionFind sets(4);
  sets.unite(0, 1);
  sets.unite(0, 1);
  sets.unite(1, 0);
  sets.unite(2, 2);
  EXPECT_TRUE(sets.connected(0, 1));
  EXPECT_FALSE(sets.connected(0, 2));
  EXPECT_TRUE(sets.connected(2, 2));
}

TEST(UnionFind, ElementsOfASetShareOneRepresentative) {
  UnionFind sets(8);
  for (int i = 0; i + 1 < 8; ++i) {
    sets.unite(i, i + 1);
  }
  const int root = sets.find(0);
  for (int i = 0; i < 8; ++i) {
    EXPECT_EQ(sets.find(i), root);
  }
}

TEST(UnionFind, SurvivesALongChain) {
  // find() is recursive: joining by rank has to keep the trees shallow
  const int size = 200000;
  UnionFind sets(size);
  for (int i = 0; i + 1 < size; ++i) {
    sets.unite(i, i + 1);
  }
  EXPECT_TRUE(sets.connected(0, size - 1));
}

// ===================== box-drawing characters =====================

TEST(BoxDrawingChars, NoWallsIsABlank) {
  EXPECT_EQ(get_box_drawing_char(false, false, false, false), " ");
}

TEST(BoxDrawingChars, StraightLinesAndCross) {
  EXPECT_EQ(get_box_drawing_char(true, true, false, false), "│");
  EXPECT_EQ(get_box_drawing_char(false, false, true, true), "─");
  EXPECT_EQ(get_box_drawing_char(true, true, true, true), "┼");
}

TEST(BoxDrawingChars, Corners) {
  EXPECT_EQ(get_box_drawing_char(false, true, true, false), "┌");
  EXPECT_EQ(get_box_drawing_char(false, true, false, true), "┐");
  EXPECT_EQ(get_box_drawing_char(true, false, true, false), "└");
  EXPECT_EQ(get_box_drawing_char(true, false, false, true), "┘");
}

TEST(BoxDrawingChars, EveryCombinationHasItsOwnSymbol) {
  std::set<std::string_view> symbols;
  for (int mask = 0; mask < 16; ++mask) {
    const auto symbol = get_box_drawing_char(mask & 8, mask & 4, mask & 2, mask & 1);
    EXPECT_FALSE(symbol.empty());
    symbols.insert(symbol);
  }
  EXPECT_EQ(symbols.size(), 16u);
}
