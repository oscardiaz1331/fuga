#include "fuga_eval/geometry.hpp"

#include <cmath>

double fuga_eval::distance(Point a, Point b)
{
  return std::hypot(b.x - a.x, b.y - a.y);
}

bool fuga_eval::is_outside(Point p, const Bounds & bounds, double margin)
{
  return p.x < bounds.x_min - margin || p.x > bounds.x_max + margin ||
         p.y < bounds.y_min - margin || p.y > bounds.y_max + margin;
}
