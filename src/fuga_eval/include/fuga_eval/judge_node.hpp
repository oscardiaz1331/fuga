#pragma once

#include "fuga_eval/judge.hpp"
#include "fuga_eval/meta.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include <memory>
#include <rclcpp/rclcpp.hpp>
#include <string>

namespace fuga_eval
{

class JudgeNode : public rclcpp::Node {
public:
  JudgeNode();

private:
  std::unique_ptr<Judge> judge_;
  Meta meta_;
  std::string meta_file_, csv_file_, algorithm_;
  // Guardar el mapa SLAM al terminar y compararlo con el PGM verdad
  bool save_map_ = true;
  std::string map_dir_;
  double iou_tolerance_ = 0.0;

  rclcpp::Subscription<geometry_msgs::msg::PoseStamped>::SharedPtr
    subscription_;

  void callback(const geometry_msgs::msg::PoseStamped & msg);
  // map_saver_cli + map_iou.py. Bloquea unos segundos; no lanza excepciones
  void capture_map(const std::string & name);
  std::string run_and_read(const std::string & command, bool & ok);
};

} // namespace fuga_eval
