#pragma once

#include <ctime>
#include <string>
#include <string_view>

#include "fuga_eval/judge.hpp"
#include "fuga_eval/meta.hpp"

namespace fuga_eval
{

// Nombres de las columnas, sin '\n':
// date,algorithm,seed,width,height,difficulty,result,time_s,path_length_m,
// shortest_m,exit_x,exit_y
std::string csv_header();

// Una fila con las columnas de csv_header(), sin '\n'. Decimales con punto y 3
// cifras. Los campos que no hay (seed, difficulty, shortest_m o la salida si
// no ha salido) quedan vacios
std::string csv_row(
  std::string_view date, std::string_view algorithm,
  const Meta & meta, const Outcome & outcome);

// Entre comillas, y con las comillas dobladas, si tiene ',', '"' o saltos de
// linea (RFC 4180). Si no, igual
std::string csv_escape(std::string_view field);

// Hora local en ISO 8601, p. ej. 2026-10-24T16:03:02
std::string format_local_time(std::time_t t);

// Anade row al final del fichero, creando las carpetas que falten. Si el
// fichero no existe o esta vacio, escribe antes la cabecera.
// Lanza std::runtime_error si no puede escribir
void append_row(const std::string & path, const std::string & row);

} // namespace fuga_eval
