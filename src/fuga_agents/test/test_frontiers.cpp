#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <vector>

#include "fuga_agents/frontiers.hpp"

using fuga_agents::choose_frontier;
using fuga_agents::find_frontiers;
using fuga_agents::Frontier;
using fuga_agents::has_frontier_near;
using fuga_agents::path_distances;

namespace
{
// Mapa dibujado en ASCII: '.' libre (0), '#' ocupado (100), '?' desconocido (-1).
// La primera cadena es la fila y = 0, igual que en OccupancyGrid::data.
struct Map {
  int width = 0;
  int height = 0;
  std::vector<int8_t> data;

  int8_t at(int x, int y) const {return data[y * width + x];}
};

Map parse(const std::vector<std::string> & rows)
{
  Map map;
  map.height = static_cast<int>(rows.size());
  map.width = static_cast<int>(rows.front().size());
  for (const std::string & row : rows) {
    for (const char c : row) {
      map.data.push_back(c == '.' ? 0 : c == '#' ? 100 : -1);
    }
  }
  return map;
}

std::vector<Frontier> frontiers_of(const Map & map, int min_size = 1)
{
  return find_frontiers(map.width, map.height, map.data, min_size);
}

bool contains(const Frontier & f, int x, int y, int width)
{
  return std::find(f.cells.begin(), f.cells.end(), y * width + x) != f.cells.end();
}

// choose_frontier con las distancias por camino desde el robot en (robot_x, robot_y)
std::optional<Frontier> choose_from(const Map & map, const std::vector<Frontier> & fs,
                                    int robot_x, int robot_y,
                                    double distance_weight = 1.0, double size_weight = 0.1)
{
  const auto distances = path_distances(map.width, map.height, map.data, robot_x, robot_y);
  return choose_frontier(fs, distances, map.width, distance_weight, size_weight);
}
} // namespace

// ---------------------------------------------------------------- find_frontiers

TEST(FindFrontiers, MapaConocidoNoTieneFronteras) {
  const Map map = parse({
    "#####",
    "#...#",
    "#####",
  });
  EXPECT_TRUE(frontiers_of(map).empty());
}

TEST(FindFrontiers, MapaDesconocidoNoTieneFronteras) {
  const Map map = parse({
    "???",
    "???",
  });
  EXPECT_TRUE(frontiers_of(map).empty());
}

TEST(FindFrontiers, MapasDeUnaCeldaNoSeSalenDelGrid) {
  EXPECT_TRUE(frontiers_of(parse({"."})).empty());
  EXPECT_TRUE(frontiers_of(parse({"?"})).empty());
  EXPECT_TRUE(frontiers_of(parse({"#"})).empty());
}

TEST(FindFrontiers, FinalDePasilloAbierto) {
  // No es cuadrado (5x3): si se cambiaran x e y, el indice saldria mal.
  const Map map = parse({
    "#####",
    "#...?",
    "#####",
  });
  const auto fs = frontiers_of(map);
  ASSERT_EQ(fs.size(), 1u);
  EXPECT_EQ(fs[0].cells, std::vector<int>{1 * 5 + 3});
  EXPECT_EQ(fs[0].target_x, 3);
  EXPECT_EQ(fs[0].target_y, 1);
}

TEST(FindFrontiers, LineaDeCeldasEsUnSoloGrupo) {
  const Map map = parse({
    "???????",
    "#.....#",
    "#.....#",
    "#######",
  });
  const auto fs = frontiers_of(map);
  ASSERT_EQ(fs.size(), 1u);
  EXPECT_EQ(fs[0].cells.size(), 5u);
  for (int x = 1; x <= 5; ++x) {
    EXPECT_TRUE(contains(fs[0], x, 1, map.width)) << "falta x=" << x;
  }
  // Objetivo en el centro de la linea
  EXPECT_EQ(fs[0].target_x, 3);
  EXPECT_EQ(fs[0].target_y, 1);
}

TEST(FindFrontiers, ZonasSeparadasSonGruposDistintos) {
  const Map map = parse({
    "#######",
    "?.....?",
    "#######",
  });
  const auto fs = frontiers_of(map);
  ASSERT_EQ(fs.size(), 2u);
  EXPECT_EQ(fs[0].cells.size(), 1u);
  EXPECT_EQ(fs[1].cells.size(), 1u);
  EXPECT_TRUE(contains(fs[0], 1, 1, map.width) || contains(fs[1], 1, 1, map.width));
  EXPECT_TRUE(contains(fs[0], 5, 1, map.width) || contains(fs[1], 5, 1, map.width));
}

TEST(FindFrontiers, CeldasEnDiagonalSeAgrupan) {
  // (1,0) y (2,1) solo se tocan en diagonal: con 8-conexion son un grupo.
  const Map map = parse({
    "?.##",
    "#?.#",
    "####",
  });
  const auto fs = frontiers_of(map);
  ASSERT_EQ(fs.size(), 1u);
  EXPECT_EQ(fs[0].cells.size(), 2u);
}

TEST(FindFrontiers, DesconocidoSoloEnDiagonalNoEsFrontera) {
  // La deteccion usa 4 vecinos: una esquina desconocida no basta.
  const Map map = parse({
    "?#",
    "#.",
  });
  EXPECT_TRUE(frontiers_of(map).empty());
}

TEST(FindFrontiers, ParedJuntoADesconocidoNoEsFrontera) {
  const Map map = parse({
    "?#.",
  });
  EXPECT_TRUE(frontiers_of(map).empty());
}

TEST(FindFrontiers, MinSizeDescartaGruposPequenos) {
  const Map map = parse({
    "???????",
    "#.....#",
    "#.....#",
    "#.....#",
    "#.....?",
    "#######",
  });
  EXPECT_EQ(frontiers_of(map, 1).size(), 2u);  // linea de 5 + celda suelta (5,4)

  const auto grandes = frontiers_of(map, 2);
  ASSERT_EQ(grandes.size(), 1u);
  EXPECT_EQ(grandes[0].cells.size(), 5u);

  EXPECT_EQ(frontiers_of(map, 5).size(), 1u);  // el limite es inclusivo
  EXPECT_TRUE(frontiers_of(map, 6).empty());
}

TEST(FindFrontiers, ObjetivoEsCeldaLibreDelGrupoAunqueElCentroideCaigaEnPared) {
  // Frontera en L: el centroide (1.6, 1.6) cae cerca de (2,2), que es pared.
  const Map map = parse({
    "?????",
    "?...#",
    "?.###",
    "?.###",
    "#####",
  });
  const auto fs = frontiers_of(map);
  ASSERT_EQ(fs.size(), 1u);
  EXPECT_EQ(fs[0].cells.size(), 5u);
  const Frontier & f = fs[0];
  EXPECT_TRUE(contains(f, f.target_x, f.target_y, map.width));
  EXPECT_EQ(map.at(f.target_x, f.target_y), 0);
}

TEST(FindFrontiers, ConHolguraElObjetivoSeApartaDeLaPared) {
  // El centroide (4,1) tiene una pared justo debajo; con holgura 1 el objetivo
  // pasa a una celda vecina sin paredes a distancia 1
  const Map map = parse({
    "?????????",
    "#.......#",
    "#...#...#",
    "#.......#",
    "#########",
  });
  const auto sin_holgura = find_frontiers(map.width, map.height, map.data, 1, 0);
  ASSERT_EQ(sin_holgura.size(), 1u);
  EXPECT_EQ(sin_holgura[0].target_x, 4);

  const auto fs = find_frontiers(map.width, map.height, map.data, 1, 1);
  ASSERT_EQ(fs.size(), 1u);
  EXPECT_EQ(fs[0].cells.size(), 7u);  // el grupo no cambia, solo el objetivo
  EXPECT_TRUE(fs[0].target_x == 3 || fs[0].target_x == 5) << fs[0].target_x;
  EXPECT_EQ(fs[0].target_y, 1);
}

TEST(FindFrontiers, ConHolguraSeDescartanFronterasPegadasAParedes) {
  // Hueco desconocido en un muro (SLAM no vio esas celdas): las fronteras de
  // los dos lados estan pegadas a la pared y el robot no cabe
  const Map map = parse({
    "#####",
    "#...#",
    "##?##",
    "#...#",
    "#####",
  });
  EXPECT_EQ(find_frontiers(map.width, map.height, map.data, 1, 0).size(), 2u);
  EXPECT_TRUE(find_frontiers(map.width, map.height, map.data, 1, 1).empty());
}

// ---------------------------------------------------------------- has_frontier_near

TEST(HasFrontierNear, CentroEnFronteraEsTrue) {
  const Map map = parse({
    "#####",
    "#...?",
    "#####",
  });
  EXPECT_TRUE(has_frontier_near(map.width, map.height, map.data, 3, 1, 0));
}

TEST(HasFrontierNear, FronteraDesplazadaDentroDelRadio) {
  // El objetivo era (1,1), pero el robot descubrio esa zona y la frontera esta en (3,1)
  const Map map = parse({
    "#####",
    "#...?",
    "#####",
  });
  EXPECT_FALSE(has_frontier_near(map.width, map.height, map.data, 1, 1, 1));
  EXPECT_TRUE(has_frontier_near(map.width, map.height, map.data, 1, 1, 2));
}

TEST(HasFrontierNear, RadioEsVentanaCuadrada) {
  // (3,1) esta en diagonal a distancia 2 de (1,3): dentro de la ventana de radio 2
  const Map map = parse({
    "#####",
    "#...?",
    "#...#",
    "#...#",
    "#####",
  });
  EXPECT_TRUE(has_frontier_near(map.width, map.height, map.data, 1, 3, 2));
  EXPECT_FALSE(has_frontier_near(map.width, map.height, map.data, 1, 3, 1));
}

TEST(HasFrontierNear, FronteraDesaparecidaEsFalse) {
  const Map map = parse({
    "#####",
    "#...#",
    "#####",
  });
  EXPECT_FALSE(has_frontier_near(map.width, map.height, map.data, 2, 1, 10));
}

TEST(HasFrontierNear, CentroFueraDelGridEsFalse) {
  // Puede pasar si el mapa cambia de origen o de tamano entre ticks
  const Map map = parse({
    "?..",
  });
  EXPECT_FALSE(has_frontier_near(map.width, map.height, map.data, -1, 0, 5));
  EXPECT_FALSE(has_frontier_near(map.width, map.height, map.data, 3, 0, 5));
  EXPECT_FALSE(has_frontier_near(map.width, map.height, map.data, 1, 1, 5));
}

TEST(HasFrontierNear, VentanaSeRecortaEnLosBordes) {
  // Radio mucho mayor que el mapa: no debe leer fuera del grid
  const Map map = parse({
    "?.",
    "..",
  });
  EXPECT_TRUE(has_frontier_near(map.width, map.height, map.data, 1, 1, 100));
}

// ---------------------------------------------------------------- choose_frontier

TEST(ChooseFrontier, SinFronterasDevuelveNullopt) {
  EXPECT_FALSE(choose_frontier({}, {}, 0).has_value());
}

TEST(ChooseFrontier, AIgualTamanoEligeLaMasCercana) {
  const Map map = parse({
    "#######",
    "?.....?",
    "#######",
  });
  const auto fs = frontiers_of(map);

  const auto izquierda = choose_from(map, fs, 2, 1);
  ASSERT_TRUE(izquierda.has_value());
  EXPECT_EQ(izquierda->target_x, 1);

  const auto derecha = choose_from(map, fs, 4, 1);
  ASSERT_TRUE(derecha.has_value());
  EXPECT_EQ(derecha->target_x, 5);
}

TEST(ChooseFrontier, ElTamanoPuedeCompensarLaDistancia) {
  // Robot en (5,3): la celda suelta (5,4) esta a 1 por camino; la linea grande
  // (objetivo (3,1)) a 4 (2 a la izquierda y 2 arriba, sin diagonales).
  const Map map = parse({
    "???????",
    "#.....#",
    "#.....#",
    "#.....#",
    "#.....?",
    "#######",
  });
  const auto fs = frontiers_of(map);
  ASSERT_EQ(fs.size(), 2u);

  // Pesos por defecto (1.0, 0.1): pequena 1 - 0.1 = 0.9 < grande 4 - 0.5 = 3.5
  const auto por_defecto = choose_from(map, fs, 5, 3);
  ASSERT_TRUE(por_defecto.has_value());
  EXPECT_EQ(por_defecto->target_x, 5);
  EXPECT_EQ(por_defecto->target_y, 4);

  // Con mas peso al tamano: grande 4 - 5 = -1 < pequena 1 - 1 = 0
  const auto por_tamano = choose_from(map, fs, 5, 3, 1.0, 1.0);
  ASSERT_TRUE(por_tamano.has_value());
  EXPECT_EQ(por_tamano->cells.size(), 5u);
}

TEST(ChooseFrontier, UsaLaDistanciaPorCaminoNoEnLineaRecta) {
  // Robot en (2,1). (0,1) esta a 2 en linea recta pero hay una pared en medio:
  // por camino son 6 (bajar, rodear y subir). (6,1) esta a 4 por el pasillo.
  const Map map = parse({
    "?#######",
    ".#.....?",
    ".#.###.#",
    ".......#",
    "########",
  });
  const auto fs = frontiers_of(map);
  ASSERT_EQ(fs.size(), 2u);

  const auto elegida = choose_from(map, fs, 2, 1, 1.0, 0.0);
  ASSERT_TRUE(elegida.has_value());
  EXPECT_EQ(elegida->target_x, 6);
  EXPECT_EQ(elegida->target_y, 1);
}

TEST(ChooseFrontier, IgnoraFronterasSinCamino) {
  // La frontera (3,1) esta al otro lado de una pared, sin camino por celdas libres
  const Map map = parse({
    "#####",
    "#.#.?",
    "#####",
  });
  const auto fs = frontiers_of(map);
  ASSERT_EQ(fs.size(), 1u);
  EXPECT_FALSE(choose_from(map, fs, 1, 1).has_value());
}

// ---------------------------------------------------------------- path_distances

TEST(PathDistances, PasilloRecto) {
  const Map map = parse({
    ".....",
  });
  const auto d = path_distances(map.width, map.height, map.data, 1, 0);
  EXPECT_EQ(d, (std::vector<int>{1, 0, 1, 2, 3}));
}

TEST(PathDistances, LasParedesObliganARodear) {
  const Map map = parse({
    ".#.",
    "...",
  });
  const auto d = path_distances(map.width, map.height, map.data, 0, 0);
  EXPECT_EQ(d[0 * 3 + 2], 4);   // (2,0): baja, dos a la derecha y sube
  EXPECT_EQ(d[0 * 3 + 1], -1);  // la pared no se alcanza
}

TEST(PathDistances, LoDesconocidoNoSeAtraviesa) {
  const Map map = parse({
    "..?..",
  });
  const auto d = path_distances(map.width, map.height, map.data, 0, 0);
  EXPECT_EQ(d[1], 1);
  EXPECT_EQ(d[2], -1);
  EXPECT_EQ(d[3], -1);
}

TEST(PathDistances, NoCortaEsquinasEnDiagonal) {
  // Dos paredes que solo se tocan por la esquina cierran el paso
  const Map map = parse({
    ".#",
    "#.",
  });
  const auto d = path_distances(map.width, map.height, map.data, 0, 0);
  EXPECT_EQ(d[1 * 2 + 1], -1);
}

TEST(PathDistances, ElInicioSeExpandeAunqueNoSeaLibre) {
  // SLAM puede marcar como pared o desconocida la celda donde esta el robot
  const Map map = parse({
    "#..",
  });
  const auto d = path_distances(map.width, map.height, map.data, 0, 0);
  EXPECT_EQ(d, (std::vector<int>{0, 1, 2}));
}

TEST(PathDistances, InicioFueraDelGridNoAlcanzaNada) {
  const Map map = parse({
    "...",
  });
  const auto d = path_distances(map.width, map.height, map.data, -1, 0);
  EXPECT_EQ(d, (std::vector<int>{-1, -1, -1}));
}

// ---------------------------------------------------------------- find_exits

std::vector<fuga_agents::ExitCandidate> exits_of(const Map & map, int min_size = 1)
{
  return fuga_agents::find_exits(map.width, map.height, map.data, min_size);
}

TEST(FindExits, SinMurosNoHaySalidas) {
  const Map map = parse({
    "...",
    "...",
  });
  EXPECT_TRUE(exits_of(map).empty());
}

TEST(FindExits, CajaCerradaNoTieneSalidas) {
  const Map map = parse({
    "####",
    "#..#",
    "####",
  });
  EXPECT_TRUE(exits_of(map).empty());
}

TEST(FindExits, HuecoAlSurConLibreFuera) {
  // Fila y = 0 primero: el hueco esta en la fila 1 y el lidar ha visto la fila 0
  const Map map = parse({
    "??...???",
    "##...###",
    "#......#",
    "########",
  });
  const auto exits = exits_of(map);
  ASSERT_EQ(exits.size(), 1u);
  EXPECT_EQ(exits[0].door_x, 3);  // centro de x = 2..4
  EXPECT_EQ(exits[0].door_y, 0);
  EXPECT_EQ(exits[0].out_dx, 0);
  EXPECT_EQ(exits[0].out_dy, -1);
  EXPECT_EQ(exits[0].size, 3);
}

TEST(FindExits, HuecoAlEste) {
  const Map map = parse({
    "#####?",
    "#...#?",
    "#.....",
    "#...#?",
    "#####?",
  });
  const auto exits = exits_of(map);
  ASSERT_EQ(exits.size(), 1u);
  EXPECT_EQ(exits[0].door_x, 5);
  EXPECT_EQ(exits[0].door_y, 2);
  EXPECT_EQ(exits[0].out_dx, 1);
  EXPECT_EQ(exits[0].out_dy, 0);
}

TEST(FindExits, HuecoSinVerFueraNoEsSalida) {
  // Hueco en el muro pero fuera todo desconocido: puede ser muro que SLAM no ha visto
  const Map map = parse({
    "????????",
    "##...###",
    "#......#",
    "########",
  });
  EXPECT_TRUE(exits_of(map).empty());
}

TEST(FindExits, DescartaGruposPequenos) {
  const Map map = parse({
    "#####?",
    "#...#?",
    "#.....",
    "#...#?",
    "#####?",
  });
  EXPECT_TRUE(exits_of(map, 3).empty());
}

TEST(FindExits, DosSalidasSonDosCandidatos) {
  const Map map = parse({
    "?..???..?",
    "#..###..#",
    "#.......#",
    "#########",
  });
  EXPECT_EQ(exits_of(map).size(), 2u);
}
