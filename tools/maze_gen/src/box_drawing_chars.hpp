#pragma once
#include <string_view>

namespace maze_gen
{

// The box-drawing character with a stroke towards each side that has a wall
std::string_view get_box_drawing_char(bool n, bool s, bool e, bool w);

}  // namespace maze_gen
