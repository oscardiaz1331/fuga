#include "maze_gen/maze_generator.hpp"
#include <algorithm>
#include <stdexcept>
#include <string>
#include <utility>
#include "algorithm_registry.hpp"
#include "dead_end_braider.hpp"
#include "maze_exit_generator.hpp"
#include "region_connector.hpp"
#include "room_generator.hpp"

namespace maze_gen
{

namespace
{

// Each phase draws from its own random sequence, all derived from the same seed
constexpr uint32_t kAlgorithmStream = 0;
constexpr uint32_t kBraidStream = 1;
constexpr uint32_t kRoomStream = 2;
constexpr uint32_t kExitStream = 3;

void require_range(const char * name, int value, int min, int max)
{
  if (value < min || value > max) {
    throw std::invalid_argument(
            std::string("maze_gen: ") + name + " must be between " + std::to_string(min) +
            " and " + std::to_string(max) + ", got " + std::to_string(value));
  }
}

void require_probability(const char * name, double value)
{
  // Written so that NaN is rejected too
  if (!(value >= 0.0 && value <= 1.0)) {
    throw std::invalid_argument(
            std::string("maze_gen: ") + name + " must be between 0 and 1, got " +
            std::to_string(value));
  }
}

// Everything is checked before the grid is touched, so a bad option changes nothing
void validate(const MazeStrategyOptions & options, const MazeGrid & grid)
{
  require_range("difficulty", options.difficulty, 0, 10);
  require_range("room_level", options.room_level, 0, 10);
  require_range("cave_level", options.cave_level, 0, 10);
  require_probability("growing_tree_newest_ratio", options.growing_tree_newest_ratio);
  require_probability("sidewinder_east_probability", options.sidewinder_east_probability);
  require_probability("ellers_join_probability", options.ellers_join_probability);

  const auto & fixed = options.fixed_exits;
  for (size_t i = 0; i < fixed.size(); ++i) {
    const Exit & exit = fixed[i];
    const std::string where = "maze_gen: fixed exit #" + std::to_string(i) + " at (" +
      std::to_string(exit.x) + ", " + std::to_string(exit.y) + ")";
    const bool known_direction =
      std::find(kDirections.begin(), kDirections.end(), exit.dir) != kDirections.end();
    if (!known_direction) {
      throw std::invalid_argument(where + " has a direction that is not N, E, S or W");
    }
    if (!grid.in_bounds(exit.x, exit.y)) {
      throw std::invalid_argument(where + " is outside the grid");
    }
    const auto [nx, ny] = MazeGrid::neighbor(exit.x, exit.y, exit.dir);
    if (grid.in_bounds(nx, ny)) {
      throw std::invalid_argument(where + " does not point outside the grid");
    }
    if (std::find(fixed.begin(), fixed.begin() + i, exit) != fixed.begin() + i) {
      throw std::invalid_argument(where + " is repeated");
    }
  }

  if (options.num_random_exits < 0) {
    throw std::invalid_argument(
            "maze_gen: num_random_exits cannot be negative, got " +
            std::to_string(options.num_random_exits));
  }
  const std::vector<Exit> slots = MazeExitGenerator::random_exit_slots(grid);
  const auto free_slots = std::count_if(
    slots.begin(), slots.end(), [&fixed](const Exit & slot) {
      return std::find(fixed.begin(), fixed.end(), slot) == fixed.end();
    });
  if (options.num_random_exits > free_slots) {
    throw std::invalid_argument(
            "maze_gen: num_random_exits is " + std::to_string(options.num_random_exits) +
            " but this grid only has room for " + std::to_string(free_slots));
  }
}

}  // namespace

struct MazeGenerator::Impl
{
  const AlgorithmInfo * info = nullptr;
  std::unique_ptr<MazeStrategy> strategy;
  RoomGenerator rooms;
  MazeExitGenerator exits;
  RegionConnector connector;
};

MazeGenerator::MazeGenerator(MazeAlgorithm algorithm)
: impl_(std::make_unique<Impl>())
{
  set_algorithm(algorithm);
}

MazeGenerator::~MazeGenerator() = default;
MazeGenerator::MazeGenerator(MazeGenerator &&) noexcept = default;
MazeGenerator & MazeGenerator::operator=(MazeGenerator &&) noexcept = default;

void MazeGenerator::set_algorithm(MazeAlgorithm algorithm)
{
  const AlgorithmInfo & info = algorithm_info(algorithm);  // throws before changing anything
  impl_->strategy = info.create();
  impl_->info = &info;
}

MazeAlgorithm MazeGenerator::algorithm() const
{
  return impl_->info->id;
}

void MazeGenerator::generate(MazeGrid & grid, const MazeStrategyOptions & options)
{
  validate(options, grid);
  grid.reset();

  Random algorithm_rng(options.seed, kAlgorithmStream);
  impl_->strategy->generate(grid, algorithm_rng, options);

  if (impl_->info->perfect) {
    if (options.difficulty < 10) {
      Random braid_rng(options.seed, kBraidStream);
      braid(grid, braid_rng, (10 - options.difficulty) / 10.0);
    }
    // Rooms only remove walls, so they go after the algorithm: Recursive Division
    // rebuilds every wall and would erase rooms carved before it
    if (options.rooms) {
      Random room_rng(options.seed, kRoomStream);
      impl_->rooms.generate_rooms(grid, room_rng, options.room_level);
    }
  }

  Random exit_rng(options.seed, kExitStream);
  impl_->exits.generate_exits(grid, exit_rng, options);

  // A perfect maze is a single region, and braiding, rooms and exits only open walls,
  // so there is nothing to join. A cave may have an exit that landed on solid rock
  if (!impl_->info->perfect) {
    impl_->connector.connect(grid);
  }
}

MazeGrid MazeGenerator::generate(int width, int height, const MazeStrategyOptions & options)
{
  MazeGrid grid(width, height);
  generate(grid, options);
  return grid;
}

}  // namespace maze_gen
