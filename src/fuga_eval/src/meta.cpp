#include "fuga_eval/meta.hpp"

#include <stdexcept>

#include <yaml-cpp/yaml.h>

namespace
{

YAML::Node required(const YAML::Node & root, const char *key)
{
  const YAML::Node node = root[key];
  if (!node || node.IsNull()) {
    throw std::runtime_error(std::string("meta.yaml: falta '") + key + "'");
  }
  return node;
}

// Ausente o null (p. ej. shortest_path: null): vacio
template<typename T>
std::optional<T> optional_field(const YAML::Node & root, const char *key)
{
  const YAML::Node node = root[key];
  if (!node || node.IsNull()) {
    return std::nullopt;
  }
  return node.as<T>();
}

// Los errores de yaml-cpp (YAML::Exception) ya derivan de std::runtime_error
fuga_eval::Meta from_node(const YAML::Node & root)
{
  if (!root.IsMap()) {
    throw std::runtime_error("meta.yaml: se esperaba un mapa clave: valor");
  }

  fuga_eval::Meta meta;
  meta.seed = optional_field<std::uint64_t>(root, "seed");
  meta.difficulty = optional_field<int>(root, "difficulty");
  meta.width = required(root, "width").as<int>();
  meta.height = required(root, "height").as<int>();

  const YAML::Node bounds = required(root, "bounds");
  if (!bounds.IsSequence() || bounds.size() != 4) {
    throw std::runtime_error(
        "meta.yaml: bounds debe ser [x_min, y_min, x_max, y_max]");
  }
  meta.bounds = {bounds[0].as<double>(), bounds[1].as<double>(),
    bounds[2].as<double>(), bounds[3].as<double>()};

  meta.shortest_path = optional_field<double>(root, "shortest_path");
  return meta;
}

} // namespace

fuga_eval::Meta fuga_eval::parse_meta(const std::string & yaml)
{
  return from_node(YAML::Load(yaml));
}

fuga_eval::Meta fuga_eval::load_meta(const std::string & path)
{
  return from_node(YAML::LoadFile(path));
}
