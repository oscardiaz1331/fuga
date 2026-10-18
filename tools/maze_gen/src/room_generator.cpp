#include "room_generator.hpp"
#include <algorithm>
#include <array>

namespace maze_gen
{

RoomGenerator::RoomConfig RoomGenerator::room_config(int level, int width, int height) const
{
  const int total_cells = width * height;
  const int min_dimension = std::min(width, height);

  RoomConfig config;

  // Room sizes scale with the level and with the size of the maze
  config.min_room_size = std::max(2, min_dimension / 15 + level / 3);
  config.max_room_size = std::max(config.min_room_size + 1, min_dimension / 8 + level / 2);

  // Number of rooms
  const double base_density = 0.02 + (level * 0.008);  // 2% to 10%
  const int base_rooms = static_cast<int>(total_cells * base_density / 20);
  config.min_rooms = std::max(1, base_rooms);
  config.max_rooms = std::max(config.min_rooms, base_rooms + level / 2);

  config.room_density = std::min(0.25, 0.05 + level * 0.02);
  return config;
}

std::vector<Room> RoomGenerator::place_rooms(
  const MazeGrid & grid, Random & rng, int level) const
{
  const int width = grid.width();
  const int height = grid.height();
  std::vector<Room> rooms;
  RoomConfig config = room_config(level, width, height);

  // A room keeps one free cell to the north and west borders and two to the south and
  // east ones, so its side can be at most this. Smaller mazes get no rooms
  const int max_fit = std::min(width, height) - 3;
  if (max_fit < config.min_room_size) {
    return rooms;
  }
  config.max_room_size = std::min(config.max_room_size, max_fit);

  const int target_rooms = rng.uniform_int(config.min_rooms, config.max_rooms);
  const int max_room_area = static_cast<int>(grid.cell_count() * config.room_density);
  int room_area = 0;

  // Random positions, with a cap on the attempts so that a crowded maze cannot loop forever
  const int max_attempts = target_rooms * 50;
  for (int attempt = 0;
    attempt < max_attempts && static_cast<int>(rooms.size()) < target_rooms; ++attempt)
  {
    const int room_width = rng.uniform_int(config.min_room_size, config.max_room_size);
    const int room_height = rng.uniform_int(config.min_room_size, config.max_room_size);
    const int x = rng.uniform_int(1, width - room_width - 2);
    const int y = rng.uniform_int(1, height - room_height - 2);
    const Room candidate(x, y, room_width, room_height);

    const bool fits_area = room_area + room_width * room_height <= max_room_area;
    const bool touches_another = std::any_of(
      rooms.begin(), rooms.end(),
      [&candidate](const Room & other) {return candidate.overlaps(other);});
    if (fits_area && !touches_another) {
      rooms.push_back(candidate);
      room_area += room_width * room_height;
    }
  }
  return rooms;
}

void RoomGenerator::generate_rooms(MazeGrid & grid, Random & rng, int level) const
{
  const std::vector<Room> rooms = place_rooms(grid, rng, level);
  for (const Room & room : rooms) {
    clear_room_walls(grid, room);
  }
  for (const Room & room : rooms) {
    create_room_doors(grid, rng, room);
  }
}

void RoomGenerator::clear_room_walls(MazeGrid & grid, const Room & room) const
{
  const auto [left, top] = room.top_left();
  for (int y = top; y < top + room.height(); ++y) {
    for (int x = left; x < left + room.width(); ++x) {
      if (x < left + room.width() - 1) {
        grid.carve(x, y, Dir::E);
      }
      if (y < top + room.height() - 1) {
        grid.carve(x, y, Dir::S);
      }
    }
  }
}

void RoomGenerator::create_room_doors(MazeGrid & grid, Random & rng, const Room & room) const
{
  std::vector<std::pair<int, int>> perimeter = room.perimeter_cells();
  rng.shuffle(perimeter.begin(), perimeter.end());

  // 1 to 3 doors, depending on the size of the room
  const int doors = std::min(3, std::max(1, (room.width() + room.height()) / 8));

  int doors_made = 0;
  for (const auto & [x, y] : perimeter) {
    if (doors_made >= doors) {
      break;
    }
    std::array<Dir, 4> directions = kDirections;
    rng.shuffle(directions.begin(), directions.end());
    for (Dir dir : directions) {
      const auto [nx, ny] = MazeGrid::neighbor(x, y, dir);
      if (!room.contains(nx, ny) && grid.carve(x, y, dir)) {
        ++doors_made;
        break;
      }
    }
  }
}

}  // namespace maze_gen
