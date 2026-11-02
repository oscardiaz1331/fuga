#pragma once

#include "fuga_agents/fuga_agent.hpp"
#include "fuga_cpp_navigate_to_pose/navigate_to_pose.hpp"
#include "nav_msgs/msg/occupancy_grid.hpp"
#include "tf2_ros/buffer.h"
#include "tf2_ros/transform_listener.h"

namespace fuga_agents
{
// Explora el mapa de SLAM yendo de frontera en frontera (celdas libres junto a zonas
// desconocidas) con Nav2. No publica en /cmd_vel: el movimiento lo hace Nav2.
class FrontierExplorer : public FugaAgent
{
public:
  FrontierExplorer();

private:
  struct Point
  {
    double x;
    double y;
  };

  // Lo llama el timer. Si no hay goal en curso, manda uno (send_next_goal). Si hay uno
  // y su frontera ha desaparecido, manda el siguiente sin cancelar el actual, para que
  // el robot no frene; solo cancela si no queda ninguna frontera. Comprueba el future
  // sin esperar, para no bloquear el executor.
  void decide();

  enum class GoalChoice { kSent, kNoFrontier, kNoPose };
  // Elige la mejor frontera de last_map_ y la manda a Nav2. Si ya había un goal en
  // curso, Nav2 lo sustituye (lo aborta) sin detener el robot.
  GoalChoice send_next_goal();

  // true si en last_map_ queda alguna frontera a kTargetRadius o menos de target.
  bool is_still_frontier(const Point & target) const;

  // Solo guarda el mapa; las decisiones las toma decide() a su ritmo.
  void map_callback(const nav_msgs::msg::OccupancyGrid::SharedPtr msg);

  rclcpp::Subscription<nav_msgs::msg::OccupancyGrid>::SharedPtr subscription_;
  std::shared_ptr<fuga_cpp_navigate_to_pose::NavigateToPoseCpp> navigator_;

  fuga_cpp_navigate_to_pose::NavigateToPoseCpp::ResultFuture current_goal_result_;
  std::unique_ptr<tf2_ros::Buffer> tf_buffer_;
  std::unique_ptr<tf2_ros::TransformListener> tf_listener_;

  static constexpr double kFailedRadius = 0.5;  // m
  // m: distancia mínima del objetivo a cualquier muro. Radio del robot (0.16) más
  // margen; en pasillos de 0.9 m el centro queda a 0.45, así que siempre hay sitio
  static constexpr double kTargetClearance = 0.25;
  static constexpr int kMinFrontierSize = 3;  // celdas
  // m: si no queda frontera a esta distancia del objetivo, se cambia de rumbo. Mayor
  // que una celda porque al acercarse la frontera se desplaza un poco, y el borde
  // del mapa de SLAM parpadea
  static constexpr double kTargetRadius = 0.5;
  std::optional<Point> current_target_;  // en metros
  // Objetivos que fallaron: sus fronteras se evitan mientras haya otras. En metros:
  // sobrevive a cambios de origen del mapa. Se vacía si solo quedan fronteras vetadas
  std::vector<Point> failed_targets_;
  // true tras cancelar el goal a propósito (no quedaban fronteras): su CANCELED no
  // cuenta como frontera fallida
  bool replanning_ = false;

  nav_msgs::msg::OccupancyGrid::SharedPtr last_map_;  // nullptr hasta el primer /map
  rclcpp::TimerBase::SharedPtr timer_;                // reloj del nodo: tiempo simulado
};
}  // namespace fuga_agents