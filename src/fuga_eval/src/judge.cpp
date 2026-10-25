#include "fuga_eval/judge.hpp"

std::string_view fuga_eval::to_string(Result result)
{
  switch (result) {
    case Result::kExit:
      return "exit";
    case Result::kTimeout:
      return "timeout";
    case Result::kNoStart:
      return "no_start";
  }
  return "?";
}

fuga_eval::Outcome fuga_eval::Judge::finish(const Outcome & outcome)
{
  state_ = State::kFinished;
  return outcome;
}

std::optional<fuga_eval::Outcome> fuga_eval::Judge::update(double t, Point p)
{
  switch (state_) {
    case State::kFinished:
      return std::nullopt;

    case State::kWaiting:
      if (!origin_) {
        origin_ = p;
        first_t_ = t;
        return std::nullopt;
      }
      if (distance(*origin_, p) <= config_.start_threshold) {
        if (t - first_t_ > config_.start_timeout) {
          return finish({Result::kNoStart, 0.0, 0.0, std::nullopt});
        }
        return std::nullopt;
      }
    // Ha empezado a moverse. El cronometro arranca ahora, pero la distancia
    // se mide desde la pose inicial para no perder los primeros centimetros
      state_ = State::kRunning;
      start_t_ = t;
      path_.add(*origin_);
      [[fallthrough]];

    case State::kRunning: {
        path_.add(p);
        const double elapsed = t - start_t_;
    // Si en la misma pose sale y se pasa de tiempo, cuenta la salida
        if (is_outside(p, config_.bounds, config_.exit_margin)) {
          return finish({Result::kExit, elapsed, path_.total(), p});
        }
        if (elapsed > config_.timeout) {
          return finish({Result::kTimeout, elapsed, path_.total(), std::nullopt});
        }
        return std::nullopt;
      }
  }
  return std::nullopt;
}
