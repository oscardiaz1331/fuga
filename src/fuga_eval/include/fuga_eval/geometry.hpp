#pragma once

namespace fuga_eval
{

// Coordenadas de mundo (m)
struct Point
{
  double x = 0.0;
  double y = 0.0;
};

// Rectangulo [x_min, x_max] x [y_min, y_max] (m), como bounds en meta.yaml
struct Bounds
{
  double x_min = 0.0;
  double y_min = 0.0;
  double x_max = 0.0;
  double y_max = 0.0;
};

double distance(Point a, Point b);

// True si p esta fuera de bounds ampliado margin por cada lado. bounds son los
// ejes de los muros exteriores: con un margen mayor que medio muro mas medio
// robot, el robot esta entero fuera. Vale para cualquier salida
bool is_outside(Point p, const Bounds & bounds, double margin);

} // namespace fuga_eval
