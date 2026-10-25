#include "fuga_eval/csv.hpp"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <locale>
#include <sstream>
#include <stdexcept>

std::string fuga_eval::csv_header()
{
  return "date,algorithm,seed,width,height,difficulty,result,time_s,"
         "path_length_m,shortest_m,exit_x,exit_y";
}

std::string fuga_eval::csv_row(
  std::string_view date,
  std::string_view algorithm, const Meta & meta,
  const Outcome & outcome)
{
  std::ostringstream os;
  // Sin locale clasico, con un locale espanol saldria 3,500 y romperia el CSV
  os.imbue(std::locale::classic());
  // Solo afecta a los double: seed, width... siguen saliendo como enteros
  os << std::fixed << std::setprecision(3);
  const auto optional = [&os](const auto & value) {
      if (value) {
        os << *value;
      }
    };

  os << csv_escape(date) << ',' << csv_escape(algorithm) << ',';
  optional(meta.seed);
  os << ',' << meta.width << ',' << meta.height << ',';
  optional(meta.difficulty);
  os << ',' << to_string(outcome.result) << ',' << outcome.time_s << ','
     << outcome.path_length_m << ',';
  optional(meta.shortest_path);
  os << ',';
  if (outcome.exit_point) {
    os << outcome.exit_point->x << ',' << outcome.exit_point->y;
  } else {
    os << ',';
  }
  return os.str();
}

std::string fuga_eval::csv_escape(std::string_view field)
{
  if (field.find_first_of(",\"\n\r") == std::string_view::npos) {
    return std::string(field);
  }
  std::string escaped = "\"";
  for (const char c : field) {
    if (c == '"') {
      escaped += '"';
    }
    escaped += c;
  }
  escaped += '"';
  return escaped;
}

std::string fuga_eval::format_local_time(std::time_t t)
{
  std::tm local{};
  localtime_r(&t, &local); // POSIX; std::localtime no es thread-safe
  std::ostringstream os;
  os << std::put_time(&local, "%Y-%m-%dT%H:%M:%S");
  return os.str();
}

void fuga_eval::append_row(const std::string & path, const std::string & row)
{
  namespace fs = std::filesystem;
  const fs::path file(path);
  if (file.has_parent_path()) {
    fs::create_directories(file.parent_path());
  }

  std::error_code ec; // Si no se puede mirar, se trata como si no existiera
  const bool empty = !fs::exists(file, ec) || fs::file_size(file, ec) == 0;

  std::ofstream os(file, std::ios::app);
  if (!os) {
    throw std::runtime_error("No se puede escribir en " + path);
  }
  if (empty) {
    os << csv_header() << '\n';
  }
  os << row << '\n';
  if (!os) {
    throw std::runtime_error("Error al escribir en " + path);
  }
}
