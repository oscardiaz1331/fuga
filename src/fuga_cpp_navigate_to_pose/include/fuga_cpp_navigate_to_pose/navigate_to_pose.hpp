#pragma once

#include <chrono>
#include <future>
#include <memory>

#include "geometry_msgs/msg/pose_stamped.hpp"
#include "nav2_msgs/action/navigate_to_pose.hpp"
#include "rclcpp/duration.hpp"
#include "rclcpp/node.hpp"
#include "rclcpp_action/rclcpp_action.hpp"

namespace fuga_cpp_navigate_to_pose
{

class NavigateToPoseCpp
{
public:
  using NavigateToPose = nav2_msgs::action::NavigateToPose;
  using GoalHandle = rclcpp_action::ClientGoalHandle<NavigateToPose>;
  using ResultFuture = std::shared_future<rclcpp_action::ResultCode>;

  explicit NavigateToPoseCpp(rclcpp::Node & node);

  // Envía el goal y devuelve un future que se completa al terminar la navegación.
  // Si el servidor no está disponible o rechaza el goal, se completa con ResultCode::UNKNOWN.
  // Cada goal tiene su propio future: si se envía uno nuevo antes de que termine el
  // anterior, Nav2 aborta el anterior y su future se completa con ABORTED, sin tocar el nuevo.
  ResultFuture send_goal(const geometry_msgs::msg::PoseStamped & pose);

  // Pide a Nav2 cancelar el último goal aceptado. No espera: el future de ese goal se
  // completa con CANCELED cuando Nav2 lo confirma. Devuelve false si no hay goal en
  // curso (aún no aceptado o ya terminado): entonces no se ha pedido nada.
  bool cancel_goal() const;

private:
  // shared_ptr porque std::promise no se puede copiar y SendGoalOptions guarda los
  // callbacks en std::function, que exige lambdas copiables. Cada goal captura la suya,
  // así el resultado de un goal no puede completar el future de otro.
  using ResultPromise = std::shared_ptr<std::promise<rclcpp_action::ResultCode>>;
  void goal_response_callback(
    const GoalHandle::SharedPtr & goal_handle, const ResultPromise & promise);
  void result_callback(const GoalHandle::WrappedResult & result, const ResultPromise & promise);
  void feedback_callback(
    const GoalHandle::SharedPtr & goal_handle,
    const std::shared_ptr<const NavigateToPose::Feedback> & feedback);

  // Referencia y no shared_ptr: el nodo es el dueño de esta clase, así que siempre
  // vive más que ella. Un shared_ptr crearía un ciclo y además shared_from_this()
  // falla si se llama desde el constructor del nodo.
  rclcpp::Node & node_;
  rclcpp_action::Client<NavigateToPose>::SharedPtr action_client_;
  bool cancel_requested_ = false;
  // Goal en curso, para cancel_goal(). Se asigna al aceptarse y vuelve a nullptr
  // cuando llega su resultado (ver result_callback).
  GoalHandle::SharedPtr current_goal_handle_ = nullptr;

  static constexpr std::chrono::seconds server_timeout_{10};
  static inline const rclcpp::Duration nav_timeout_{std::chrono::seconds(30)};
};

}  // namespace fuga_cpp_navigate_to_pose
