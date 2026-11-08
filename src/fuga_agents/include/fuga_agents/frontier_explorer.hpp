#pragma once

#include <optional>
#include <string>
#include <vector>

#include "fuga_agents/fuga_agent.hpp"
#include "fuga_cpp_navigate_to_pose/navigate_to_pose.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "nav_msgs/msg/occupancy_grid.hpp"
#include "tf2_ros/buffer.h"
#include "tf2_ros/transform_listener.h"

namespace fuga_agents
{
// Explora el mapa de SLAM yendo de frontera en frontera (celdas libres junto a zonas
// desconocidas) con Nav2. En cuanto el mapa muestra una salida (hueco del muro
// exterior con espacio libre fuera, ver find_exits) va a ella: se pone delante, gira
// hacia el hueco y, si ve el AprilTag de la salida (TF exit_tag_frame), sale.
// Casi todo el movimiento lo hace Nav2. El agente solo publica en /cmd_vel para girar
// en el sitio delante de la salida, cuando Nav2 no tiene ningun goal.
// Parametros:
//   search_exits (false): buscar salidas en el mapa. Solo funciona si SLAM marca libre
//     el espacio de fuera, y slam_toolbox descarta los rayos sin eco (inf) que salen por
//     las salidas: sin eso no hay nada que encontrar
//   require_exit_tag (true): false para salir sin esperar al tag (p. ej. apriltag:=false)
//   exit_tag_frame ("exit_tag"): frame de TF que publica apriltag_ros para el tag
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
  struct Pose2D
  {
    double x;
    double y;
    double yaw;
  };
  // Salida candidata, en metros en el marco del mapa
  struct Exit
  {
    Point door;    // centro del hueco, en la linea del muro exterior
    Point out;     // vector unitario hacia fuera del laberinto
    int leave_attempts = 0;
  };

  enum class Mode
  {
    kExplore,       // de frontera en frontera
    kApproachExit,  // Nav2 lleva el robot delante de la salida
    kFaceExit,      // girando hacia el hueco y esperando a ver el tag
    kLeave,         // Nav2 saca el robot por la salida
    kDone,          // fuera: no queda nada que hacer
  };

  void do_step() override;
  // FugaAgent saca el robot por la salida: se cancela el goal de Nav2 para que su
  // controlador deje de publicar en cmd_vel
  void on_exit_start() override;

  // Si hay goal en curso, sigue con el, salvo que su frontera haya desaparecido: entonces
  // manda el siguiente sin cancelar el actual, para que el robot no frene. Solo cancela
  // si no queda ninguna frontera. Antes de todo mira si hay una salida a la que ir.
  void explore();
  void approach_exit();
  void face_exit();
  void leave();

  enum class GoalChoice { kSent, kNoFrontier, kNoPose };
  // Elige la mejor frontera de last_map_ y la manda a Nav2. Si ya habia un goal en
  // curso, Nav2 lo sustituye (lo aborta) sin detener el robot.
  GoalChoice send_next_goal();

  // Si el mapa muestra una salida no descartada, va a la mas cercana y devuelve true
  bool start_exit();
  void send_leave_goal();
  // Descarta la salida actual para siempre y vuelve a explorar
  void reject_exit(const char * reason);
  // true si apriltag_ros ha publicado el tag hace poco y cerca de door
  bool tag_seen_near(const Point & door) const;
  void stop_turning();

  // Pose del robot en el marco del mapa; nullopt si aun no hay TF
  std::optional<Pose2D> robot_pose() const;
  geometry_msgs::msg::PoseStamped make_goal(const Point & p, double yaw) const;

  // true si en last_map_ queda alguna frontera a kTargetRadius o menos de target.
  bool is_still_frontier(const Point & target) const;

  // Solo guarda el mapa; las decisiones las toma decide() a su ritmo.
  void map_callback(const nav_msgs::msg::OccupancyGrid::SharedPtr msg);

  rclcpp::Subscription<nav_msgs::msg::OccupancyGrid>::SharedPtr subscription_;
  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_;
  std::shared_ptr<fuga_cpp_navigate_to_pose::NavigateToPoseCpp> navigator_;

  fuga_cpp_navigate_to_pose::NavigateToPoseCpp::ResultFuture current_goal_result_;
  std::unique_ptr<tf2_ros::Buffer> tf_buffer_;
  std::unique_ptr<tf2_ros::TransformListener> tf_listener_;

  bool search_exits_ = false;
  bool require_exit_tag_ = true;
  std::string exit_tag_frame_;

  static constexpr double kFailedRadius = 0.5;  // m
  // m: distancia minima del objetivo a cualquier muro. Radio del robot (0.16) mas
  // margen; en pasillos de 0.9 m el centro queda a 0.45, asi que siempre hay sitio
  static constexpr double kTargetClearance = 0.25;
  static constexpr int kMinFrontierSize = 3;  // celdas
  // m: si no queda frontera a esta distancia del objetivo, se cambia de rumbo. Mayor
  // que una celda porque al acercarse la frontera se desplaza un poco, y el borde
  // del mapa de SLAM parpadea
  static constexpr double kTargetRadius = 0.5;

  // Salidas. Con celdas de 1 m, a 0.5 m del hueco hacia dentro esta el centro de la
  // celda de la salida: siempre libre y con el tag a menos de 1 m (con la camara de
  // 320x240 el tag de 0.2 m se detecta bien hasta unos 2 m)
  static constexpr double kExitApproach = 0.5;  // m hacia dentro desde el hueco
  // m hacia fuera desde el hueco. El juez da la salida por buena a exit_margin (0.2 m)
  // fuera del laberinto
  static constexpr double kExitLeave = 0.8;
  static constexpr double kFaceTolerance = 0.1;  // rad
  static constexpr double kTurnGain = 1.5;       // (rad/s) / rad
  static constexpr double kMaxTurnSpeed = 1.0;   // rad/s
  // s simulados para girar y ver el tag. Media vuelta a 1 rad/s son ~3 s; el resto,
  // para que apriltag_ros procese alguna imagen ya girado
  static constexpr double kFaceTimeout = 8.0;
  // s: un TF del tag mas viejo que esto es de antes de girar, no cuenta
  static constexpr double kTagMaxAge = 1.0;
  // m: el tag esta en el centro del hueco. Si se ve mas lejos es de otra salida
  static constexpr double kTagDoorRadius = 0.5;
  static constexpr int kMaxLeaveAttempts = 3;

  Mode mode_ = Mode::kExplore;
  std::optional<Exit> exit_;  // salida a la que se va (modos de salida)
  rclcpp::Time face_start_;
  // Salidas descartadas (sin tag o Nav2 no llega). En metros, como failed_targets_
  std::vector<Point> rejected_exits_;

  std::optional<Point> current_target_;  // en metros
  // Objetivos que fallaron: sus fronteras se evitan mientras haya otras. En metros:
  // sobrevive a cambios de origen del mapa. Se vacia si solo quedan fronteras vetadas
  std::vector<Point> failed_targets_;
  // true tras cancelar el goal a proposito (no quedaban fronteras): su CANCELED no
  // cuenta como frontera fallida
  bool replanning_ = false;

  nav_msgs::msg::OccupancyGrid::SharedPtr last_map_;  // nullptr hasta el primer /map
  rclcpp::TimerBase::SharedPtr timer_;                // reloj del nodo: tiempo simulado
};
}  // namespace fuga_agents
