#include "fuga_eval/judge_node.hpp"

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <string>

#include <yaml-cpp/yaml.h>

#include "fuga_eval/csv.hpp"
#include "fuga_eval/map_capture.hpp"

fuga_eval::JudgeNode::JudgeNode()
: rclcpp::Node("judge")
{
  meta_file_ =
    declare_parameter("meta_file", std::string("/tmp/maze/meta.yaml"));
  csv_file_ =
    declare_parameter("csv_file", std::string("/tmp/maze/results.csv"));
  algorithm_ = declare_parameter("algorithm", std::string("unknown"));
  // false sin SLAM: no hay /map, y map_saver_cli fallaria tras 2 s de espera
  save_map_ = declare_parameter("save_map", true);
  map_dir_ = declare_parameter("map_dir", std::string("/tmp/maze/maps"));
  iou_tolerance_ = declare_parameter("iou_tolerance", 0.0);
  meta_ = load_meta(meta_file_);
  fuga_eval::JudgeConfig config;
  config.bounds = meta_.bounds;
  config.timeout = declare_parameter("timeout", 600.0);
  config.start_timeout = declare_parameter("start_timeout", 30.0);
  config.exit_margin = declare_parameter("exit_margin", 0.2);
  config.start_threshold = declare_parameter("start_threshold", 0.02);
  config.min_step = declare_parameter("min_step", 0.001);
  judge_ = std::make_unique<Judge>(config);

  // &JudgeNode::callback es el metodo y this el objeto sobre el que llamarlo;
  // _1 es el hueco donde ROS pone el mensaje
  subscription_ = create_subscription<geometry_msgs::msg::PoseStamped>(
      "/ground_truth/pose", 10,
      std::bind(&JudgeNode::callback, this, std::placeholders::_1));

  RCLCPP_INFO(get_logger(),
              "Juez listo: %s, laberinto %dx%d, bounds [%.2f, %.2f, %.2f, "
              "%.2f], timeout %.0f s. Esperando /ground_truth/pose",
              algorithm_.c_str(), meta_.width, meta_.height,
              config.bounds.x_min, config.bounds.y_min, config.bounds.x_max,
              config.bounds.y_max, config.timeout);
}

void fuga_eval::JudgeNode::callback(
  const geometry_msgs::msg::PoseStamped & msg)
{
  // sec + nanosec en segundos con decimales. Solo sec redondea a segundos
  // enteros; solo nanosec es la parte fraccionaria y vuelve a 0 cada segundo
  const double t = rclcpp::Time(msg.header.stamp).seconds();
  const auto outcome =
    judge_->update(t, {msg.pose.position.x, msg.pose.position.y});
  if (!outcome.has_value()) {
    return;
  }

  RCLCPP_INFO(get_logger(), "Fin: %s en %.2f s, recorrido %.2f m",
              std::string(to_string(outcome->result)).c_str(), outcome->time_s,
              outcome->path_length_m);
  const std::time_t now = std::time(nullptr);
  try {
    const std::string row =
      csv_row(format_local_time(now), algorithm_, meta_, *outcome);
    append_row(csv_file_, row);
    RCLCPP_INFO(get_logger(), "Resultado guardado en %s", csv_file_.c_str());
  } catch (const std::exception & e) {
    RCLCPP_ERROR(get_logger(), "No se pudo escribir %s: %s", csv_file_.c_str(),
                 e.what());
  }

  // Antes del shutdown: el bringup cierra SLAM cuando sale el juez, asi que
  // /map solo existe mientras este proceso siga vivo
  if (save_map_) {
    std::string name = file_stamp(now) + "_" + algorithm_;
    if (meta_.seed) {
      name += "_seed" + std::to_string(*meta_.seed);
    }
    capture_map(name);
  }

  // Hace que spin() vuelva y el proceso termine: el launch lo ve con
  // OnProcessExit y cierra todo lo demas
  rclcpp::shutdown();
}

void fuga_eval::JudgeNode::capture_map(const std::string & name)
{
  const MapCapture c =
    make_map_capture(map_dir_, name, meta_file_, iou_tolerance_);
  std::error_code ec;
  std::filesystem::create_directories(map_dir_, ec);
  if (ec) {
    RCLCPP_ERROR(get_logger(), "No se puede crear %s: %s", map_dir_.c_str(),
                 ec.message().c_str());
    return;
  }

  RCLCPP_INFO(get_logger(), "Guardando el mapa SLAM en %s.pgm",
              c.prefix.c_str());
  if (std::system(c.save_command.c_str()) != 0) {
    RCLCPP_ERROR(get_logger(),
                 "map_saver_cli ha fallado (hay SLAM? save_map:=false si no)");
    return;
  }

  bool ok = false;
  const std::string json = run_and_read(c.iou_command, ok);
  if (!ok) {
    RCLCPP_ERROR(get_logger(), "map_iou.py ha fallado");
    return;
  }
  std::ofstream(c.iou_json) << json;
  try {
    // El JSON es YAML valido: se lee con yaml-cpp, que ya es dependencia
    const YAML::Node r = YAML::Load(json);
    RCLCPP_INFO(get_logger(),
                "Mapa vs verdad: IoU %.3f, IoU explorado %.3f, precision "
                "%.3f, recall %.3f (%s)",
                r["iou"].as<double>(), r["explored_iou"].as<double>(),
                r["precision"].as<double>(), r["recall"].as<double>(),
                c.iou_json.c_str());
  } catch (const YAML::Exception & e) {
    RCLCPP_ERROR(get_logger(), "Salida de map_iou.py ilegible: %s", e.what());
  }
}

std::string fuga_eval::JudgeNode::run_and_read(
  const std::string & command,
  bool & ok)
{
  std::string out;
  FILE *pipe = popen(command.c_str(), "r");
  if (!pipe) {
    ok = false;
    return out;
  }
  char buffer[4096];
  while (const size_t n = std::fread(buffer, 1, sizeof buffer, pipe)) {
    out.append(buffer, n);
  }
  ok = pclose(pipe) == 0;
  return out;
}

int main(int argc, char **argv)
{
  rclcpp::init(argc, argv);
  int code = 0;
  try {
    rclcpp::spin(std::make_shared<fuga_eval::JudgeNode>());
  } catch (const std::exception & e) {
    // P. ej. meta.yaml no existe o esta mal formado (lo lanza load_meta)
    RCLCPP_FATAL(rclcpp::get_logger("judge"), "%s", e.what());
    code = 1;
  }
  rclcpp::shutdown(); // No hace nada si ya se llamo en callback
  return code;
}
