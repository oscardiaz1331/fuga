#pragma once
#include <vector>
#include "maze_gen/maze_grid.hpp"
#include "random.hpp"
#include "room.hpp"

namespace maze_gen
{

// Opens rectangular rooms in a maze: areas with no walls inside and a few doors
class RoomGenerator
{
public:
  // level goes from 0 to 10: more rooms and larger ones as it grows. A maze too small
  // to hold a room is left as it is. Only removes walls, never adds any.
  void generate_rooms(MazeGrid & grid, Random & rng, int level) const;

private:
  struct RoomConfig
  {
    int min_room_size;
    int max_room_size;
    int min_rooms;
    int max_rooms;
    double room_density;  // share of the maze that rooms may cover
  };

  RoomConfig room_config(int level, int width, int height) const;
  std::vector<Room> place_rooms(const MazeGrid & grid, Random & rng, int level) const;
  void clear_room_walls(MazeGrid & grid, const Room & room) const;
  void create_room_doors(MazeGrid & grid, Random & rng, const Room & room) const;
};

}  // namespace maze_gen
