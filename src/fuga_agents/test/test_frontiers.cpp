#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <vector>

#include "fuga_agents/frontiers.hpp"

using fuga_agents::choose_frontier;
using fuga_agents::find_frontiers;
using fuga_agents::Frontier;

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

// ---------------------------------------------------------------- choose_frontier

TEST(ChooseFrontier, SinFronterasDevuelveNullopt) {
  EXPECT_FALSE(choose_frontier({}, 0, 0).has_value());
}

TEST(ChooseFrontier, AIgualTamanoEligeLaMasCercana) {
  const Map map = parse({
    "#######",
    "?.....?",
    "#######",
  });
  const auto fs = frontiers_of(map);

  const auto izquierda = choose_frontier(fs, 2, 1);
  ASSERT_TRUE(izquierda.has_value());
  EXPECT_EQ(izquierda->target_x, 1);

  const auto derecha = choose_frontier(fs, 4, 1);
  ASSERT_TRUE(derecha.has_value());
  EXPECT_EQ(derecha->target_x, 5);
}

TEST(ChooseFrontier, ElTamanoPuedeCompensarLaDistancia) {
  // Robot en (5,3): la celda suelta (5,4) esta a 1, la linea grande (centro (3,1)) a ~2.83.
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

  // Pesos por defecto (1.0, 0.5): grande 2.83 - 2.5 = 0.33 < pequena 1 - 0.5 = 0.5
  const auto por_defecto = choose_frontier(fs, 5, 3);
  ASSERT_TRUE(por_defecto.has_value());
  EXPECT_EQ(por_defecto->cells.size(), 5u);

  // Sin peso al tamano, gana la mas cercana
  const auto solo_distancia = choose_frontier(fs, 5, 3, 1.0, 0.0);
  ASSERT_TRUE(solo_distancia.has_value());
  EXPECT_EQ(solo_distancia->target_x, 5);
  EXPECT_EQ(solo_distancia->target_y, 4);
}
