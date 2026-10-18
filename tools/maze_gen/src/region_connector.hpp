#pragma once
#include "maze_gen/maze_grid.hpp"

namespace maze_gen
{

// Makes sure no part of a maze is cut off from the rest
class RegionConnector
{
public:
  // Join every open region and every exit cell into a single region, following the
  // passages that already exist and breaking as few walls as it can. A cell with no
  // passage at all is solid rock, not a region, and is left alone unless it holds an
  // exit. Returns the number of walls broken.
  int connect(MazeGrid & grid) const;
};

}  // namespace maze_gen
