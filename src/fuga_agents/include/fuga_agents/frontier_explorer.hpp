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

  // Lo llama el timer. Si no hay goal en curso, elige una frontera de last_map_ y
  // la manda a Nav2. Si hay uno y su frontera ha desaparecido, lo cancela para
  // elegir otra. Comprueba el future sin esperar, para no bloquear el executor.
  void decide();

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
  // m: si no queda frontera a esta distancia del objetivo, se cambia de rumbo. Mayor
  // que una celda porque al acercarse la frontera se desplaza un poco, y el borde
  // del mapa de SLAM parpadea
  static constexpr double kTargetRadius = 0.5;
  std::optional<Point> current_target_;  // en metros
  std::vector<Point> failed_targets_;    // en metros: sobrevive a cambios de origen del mapa
  // true tras cancelar el goal a propósito: su CANCELED no cuenta como frontera fallida
  bool replanning_ = false;

  nav_msgs::msg::OccupancyGrid::SharedPtr last_map_;  // nullptr hasta el primer /map
  rclcpp::TimerBase::SharedPtr timer_;                // reloj del nodo: tiempo simulado
};
}  // namespace fuga_agents