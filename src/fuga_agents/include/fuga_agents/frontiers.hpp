#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace fuga_agents
{
// Grupo de celdas frontera conectadas. Coordenadas en celdas, no en metros.
struct Frontier {
  std::vector<int> cells;  // indices y * width + x
  int target_x = 0;        // celda del grupo mas cercana a su centroide
  int target_y = 0;
};

// Celdas libres (0) con algun vecino desconocido (-1), agrupadas por BFS
// (8-conexion). Descarta grupos con menos de min_size celdas. No modifica el grid.
std::vector<Frontier> find_frontiers(int width, int height,
                                     std::span<const int8_t> grid,
                                     int min_size = 3);

// Elige la frontera con menor coste: distance_weight * distancia - size_weight * tamano.
// Devuelve nullopt si no hay fronteras (mapa explorado).
std::optional<Frontier> choose_frontier(const std::vector<Frontier> & frontiers,
                                        int robot_x, int robot_y,
                                        double distance_weight = 1.0,
                                        double size_weight = 0.5);
} // namespace fuga_agents
