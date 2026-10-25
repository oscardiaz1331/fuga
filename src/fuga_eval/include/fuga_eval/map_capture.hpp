#pragma once

#include <ctime>
#include <string>
#include <string_view>

namespace fuga_eval
{

// Hora local apta para nombre de fichero, p. ej. 2026-10-25T17-03-02 (':' da
// problemas fuera de Linux y en algunas herramientas)
std::string file_stamp(std::time_t t);

// s entre comillas simples para /bin/sh, con las ' de dentro escapadas
std::string shell_quote(std::string_view s);

// Ficheros y comandos para guardar el mapa SLAM al acabar una ejecucion y
// compararlo con el PGM verdad
struct MapCapture
{
  std::string prefix;       // <dir>/<name>; map_saver_cli anade .pgm y .yaml
  std::string map_yaml;     // prefix + .yaml
  std::string diff_png;     // prefix + _diff.png, dibujo de la comparacion
  std::string iou_json;     // prefix + _iou.json, donde guardar el resultado
  std::string save_command; // map_saver_cli, con hora de simulacion
  std::string iou_command;  // map_iou.py --json: el resultado sale por stdout
};

// name: nombre de la ejecucion, sin extension; los '/' se cambian por '_'.
// La verdad es el maze.yaml que hay junto a meta_file (lo escribe maze2world)
MapCapture make_map_capture(
  const std::string & dir, std::string name,
  const std::string & meta_file, double tolerance);

} // namespace fuga_eval
