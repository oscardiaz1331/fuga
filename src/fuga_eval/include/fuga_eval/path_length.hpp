#pragma once

#include <optional>

#include "fuga_eval/geometry.hpp"

namespace fuga_eval
{

// Longitud del camino recorrido, sumando los tramos entre poses.
// Para que el ruido de la pose no la infle, solo cuenta un tramo cuando la
// pose se ha alejado al menos min_step del ultimo punto contado. Se mide desde
// ese punto y no desde la pose anterior: asi un robot muy lento, con pasos de
// menos de min_step, sigue sumando
class PathLength {
public:
  explicit PathLength(double min_step)
  : min_step_(min_step) {}

  void add(Point p);
  double total() const {return total_;}

private:
  double min_step_;
  std::optional<Point> anchor_; // Ultimo punto contado
  double total_ = 0.0;
};

} // namespace fuga_eval
