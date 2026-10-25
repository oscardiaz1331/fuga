#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "fuga_eval/geometry.hpp"

namespace fuga_eval
{

// Lo que el juez necesita de meta.yaml (lo escribe maze_export)
struct Meta
{
  std::optional<std::uint64_t> seed; // Solo si el laberinto se ha generado
  std::optional<int> difficulty;     // Idem
  int width = 0;                     // celdas
  int height = 0;                    // celdas
  Bounds bounds;
  // Camino mas corto hasta una salida (m). Vacio si no se llega a ninguna
  std::optional<double> shortest_path;
};

// Lanzan std::runtime_error si el YAML esta mal formado o falta un campo
// obligatorio (width, height, bounds)
Meta parse_meta(const std::string & yaml);
Meta load_meta(const std::string & path);

} // namespace fuga_eval
