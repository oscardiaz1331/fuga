#include "fuga_eval/path_length.hpp"

void fuga_eval::PathLength::add(Point p)
{
  if (!anchor_) {
    anchor_ = p;
    return;
  }
  const double step = distance(*anchor_, p);
  if (step >= min_step_) {
    total_ += step;
    anchor_ = p;
  }
}
