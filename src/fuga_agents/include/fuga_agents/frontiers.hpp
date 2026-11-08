#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace fuga_agents
{
// Grupo de celdas frontera conectadas. Coordenadas en celdas, no en metros.
struct Frontier
{
  std::vector<int> cells;  // indices y * width + x
  int target_x = 0;        // celda del grupo mas cercana a su centroide
  int target_y = 0;
};

// Celdas libres (0) con algun vecino desconocido (-1), agrupadas por BFS
// (8-conexion). Descarta grupos con menos de min_size celdas. No modifica el grid.
// clearance (celdas): el objetivo debe estar a mas de clearance celdas de cualquier
// obstaculo, para que el robot quepa. Se elige la celda con holgura mas cercana al
// centroide; si ninguna la tiene, se descarta el grupo (suele ser un hueco en un muro
// que SLAM no ha visto). Con 0 no se exige holgura.
std::vector<Frontier> find_frontiers(
  int width, int height,
  std::span<const int8_t> grid,
  int min_size = 3, int clearance = 0);

// true si alguna celda frontera cae en la ventana cuadrada de lado 2 * radius + 1
// centrada en (cx, cy). Sirve para saber si el objetivo actual sigue mereciendo la
// pena sin recalcular todas las fronteras. Si (cx, cy) esta fuera del grid, false.
bool has_frontier_near(
  int width, int height, std::span<const int8_t> grid,
  int cx, int cy, int radius);

// Distancia en celdas desde (start_x, start_y) hasta cada celda del grid, por BFS
// avanzando solo por celdas libres (0). Mismo indice que el grid; -1 si no se alcanza.
// - 4-conexion: en diagonal el camino se colaria entre dos paredes que solo se tocan
//   por una esquina.
// - Lo desconocido (-1) no se atraviesa: no se sabe si hay camino.
// - La celda de inicio se expande aunque no sea libre: SLAM puede marcar mal la
//   celda donde esta el robot.
// - Si el inicio esta fuera del grid, todo -1.
// No tiene en cuenta el radio del robot: puede pasar por huecos donde el robot no cabe.
std::vector<int> path_distances(
  int width, int height, std::span<const int8_t> grid,
  int start_x, int start_y);

// Elige la frontera con menor coste:
//   distance_weight * distancia por camino (celdas) - size_weight * tamano (celdas).
// distances sale de path_distances() desde el robot. Las fronteras sin camino se ignoran.
// Devuelve nullopt si no queda ninguna (mapa explorado o fronteras inalcanzables).
// Por defecto manda la cercania: 10 celdas de frontera valen 1 celda de camino, asi que
// el tamano solo decide entre fronteras a distancia parecida. Con mas peso al tamano el
// robot cruzaba el laberinto hacia fronteras grandes y volvia a por las pequenas.
std::optional<Frontier> choose_frontier(
  const std::vector<Frontier> & frontiers,
  std::span<const int> distances, int width,
  double distance_weight = 1.0,
  double size_weight = 0.1);

// Salida vista desde dentro: hueco del muro exterior por el que el lidar ha visto
// espacio libre fuera del laberinto. Coordenadas en celdas.
struct ExitCandidate
{
  int door_x = 0;  // celda del hueco, justo fuera de la caja de las celdas ocupadas
  int door_y = 0;
  int out_dx = 0;  // hacia fuera: (1, 0) este, (-1, 0) oeste,
  int out_dy = 0;  // (0, 1) norte (y crece), (0, -1) sur
  int size = 0;    // celdas libres de fuera que salen por este hueco
};

// Busca salidas en el mapa. El muro exterior es el borde de la caja que contiene todas
// las celdas ocupadas. Un rayo del lidar solo deja celdas libres fuera de esa caja si
// sale por un hueco sin chocar con nada: si chocara, el impacto ampliaria la caja. Asi
// que cada grupo de celdas libres de fuera (8-conexion, al menos min_size) es una
// salida, y su puerta es la celda media de las que tocan un lado de la caja.
// Puede dar falsos positivos (un pasillo mas largo que el alcance del lidar, con sus
// muros aun sin ver): por eso el agente confirma cada salida con el tag.
std::vector<ExitCandidate> find_exits(
  int width, int height, std::span<const int8_t> grid,
  int min_size = 3);
} // namespace fuga_agents
