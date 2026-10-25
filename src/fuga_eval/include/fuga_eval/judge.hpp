#pragma once

#include <optional>
#include <string_view>

#include "fuga_eval/geometry.hpp"
#include "fuga_eval/path_length.hpp"

namespace fuga_eval
{

enum class Result
{
  kExit,    // Ha salido del laberinto
  kTimeout, // Se ha movido, pero no ha salido a tiempo
  kNoStart, // No se ha movido en start_timeout
};

// Como aparece en el CSV: exit, timeout, no_start
std::string_view to_string(Result result);

struct Outcome
{
  Result result = Result::kTimeout;
  double time_s = 0.0; // Desde que empezo a moverse. 0 en kNoStart
  double path_length_m = 0.0;
  std::optional<Point> exit_point; // Solo en kExit: primera pose fuera
};

// Tiempos en s, distancias en m
struct JudgeConfig
{
  Bounds bounds;
  double timeout = 600.0;        // Desde que empieza a moverse
  double start_timeout = 30.0;   // Desde la primera pose
  double exit_margin = 0.2;      // Ver is_outside()
  double start_threshold = 0.02; // Cuanto se aleja del inicio para arrancar
  double min_step = 0.001;       // Ver PathLength
};

// Maquina de estados ESPERANDO -> CORRIENDO -> TERMINADO, sin ROS: recibe
// poses con su hora y dice cuando y como ha terminado la ejecucion.
//  - ESPERANDO: guarda la primera pose. Pasa a CORRIENDO cuando el robot se
//    aleja mas de start_threshold de ella, asi no cuenta lo que tarda en
//    arrancar. Si no lo hace en start_timeout, termina en kNoStart.
//  - CORRIENDO: acumula la distancia. Termina en kExit la primera vez que la
//    pose queda fuera del laberinto, o en kTimeout si pasa timeout.
class Judge {
public:
  explicit Judge(const JudgeConfig & config)
  : config_(config), path_(config.min_step) {}

  // Una pose y su hora (s; la de simulacion, para no depender de lo rapido que
  // vaya Gazebo). Devuelve el resultado en la llamada en la que termina;
  // antes y despues, vacio
  std::optional<Outcome> update(double t, Point p);
  bool finished() const {return state_ == State::kFinished;}

private:
  enum class State { kWaiting, kRunning, kFinished };

  Outcome finish(const Outcome & outcome);

  JudgeConfig config_;
  State state_ = State::kWaiting;
  std::optional<Point> origin_; // Primera pose recibida
  double first_t_ = 0.0;        // Hora de origin_
  double start_t_ = 0.0;        // Hora en que empezo a moverse
  PathLength path_;
};

} // namespace fuga_eval
