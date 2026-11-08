#pragma once

#include <geometry_msgs/msg/twist.hpp>
#include <optional>
#include <string>
#include <tf2/LinearMath/Transform.hpp>
#include <tf2/LinearMath/Vector3.hpp>
#include <tf2_ros/buffer.hpp>
#include <tf2_ros/transform_listener.hpp>
#include <vector>

#include "rclcpp/node.hpp"

namespace fuga_agents
{
// Base de todos los agentes. Los hijos llaman a step() en su bucle (callback o timer)
// e implementan do_step(). Si se ve el tag de una salida cerca, step() deja de llamar
// a do_step() y lleva el robot a través del hueco.
class FugaAgent : public rclcpp::Node
{
protected:
  FugaAgent(const std::string & name);

  void step();

private:
  virtual void do_step() = 0;
  // Se llama una vez al empezar a salir. Los hijos que mueven el robot por otra vía
  // (p. ej. un goal de Nav2) lo paran aquí: si no, publicarían en cmd_vel a la vez
  virtual void on_exit_start() {}

  // Transform odom -> frame, si existe y no es más antiguo que max_age (s).
  // odom y no map: sin SLAM no hay map, y para recorrer un metro la deriva no importa
  std::optional<tf2::Transform> lookup(const std::string & frame, double max_age) const;
  std::optional<tf2::Transform> check_exit_tag_transform() const;
  // Calcula exit_waypoints_. false si aún no se puede salir (tag lejos, sin pose)
  bool start_exit(const tf2::Transform & tag);
  // Un paso del controlador hacia exit_waypoints_.front()
  void drive_exit();

  static constexpr double kTagMaxAge = 5.0;        // s
  static constexpr double kMaxTagDistance = 1.5;   // más lejos, la recta al hueco puede cruzar muros
  static constexpr double kExitApproach = 0.4;     // punto delante del hueco, por dentro
  static constexpr double kExitLeave = 0.8;        // punto detrás del hueco, por fuera
  static constexpr double kWaypointTolerance = 0.1;
  static constexpr double kHeadingTolerance = 0.3;  // rad: solo avanza si mira al punto
  static constexpr double kTurnGain = 1.5;
  static constexpr double kMaxTurn = 1.0;    // rad/s
  static constexpr double kExitSpeed = 0.2;  // m/s

  std::unique_ptr<tf2_ros::Buffer> tf_exit_buffer_;
  std::unique_ptr<tf2_ros::TransformListener> tf_exit_listener_;
  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_exit_vel_;

  bool is_going_to_exit_ = false;
  std::vector<tf2::Vector3> exit_waypoints_;  // en odom; vacío al terminar
};

}  // namespace fuga_agents
