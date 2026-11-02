#include "fuga_cpp_navigate_to_pose/navigate_to_pose.hpp"

namespace fuga_cpp_navigate_to_pose
{

NavigateToPoseCpp::NavigateToPoseCpp(rclcpp::Node & node)
: node_(node)
{
  action_client_ = rclcpp_action::create_client<NavigateToPose>(&node_, "navigate_to_pose");
}

NavigateToPoseCpp::ResultFuture NavigateToPoseCpp::send_goal(
  const geometry_msgs::msg::PoseStamped & pose)
{
  result_promise_ = std::promise<rclcpp_action::ResultCode>{};
  ResultFuture future = result_promise_.get_future().share();

  if (!action_client_->wait_for_action_server(server_timeout_)) {
    RCLCPP_ERROR(
      node_.get_logger(), "Servidor de acción 'navigate_to_pose' no disponible tras %ld s",
      static_cast<long>(server_timeout_.count()));
    result_promise_.set_value(rclcpp_action::ResultCode::UNKNOWN);
    return future;
  }

  NavigateToPose::Goal goal{};
  goal.pose = pose;
  cancel_requested_ = false;

  rclcpp_action::Client<NavigateToPose>::SendGoalOptions options{};
  options.goal_response_callback = [this](const GoalHandle::SharedPtr & goal_handle) {
      goal_response_callback(goal_handle);
    };
  options.feedback_callback = [this](
    const GoalHandle::SharedPtr & goal_handle,
    const std::shared_ptr<const NavigateToPose::Feedback> & feedback) {
      feedback_callback(goal_handle, feedback);
    };
  options.result_callback = [this](const GoalHandle::WrappedResult & result) {
      result_callback(result);
    };

  action_client_->async_send_goal(goal, options);
  return future;
}

void NavigateToPoseCpp::goal_response_callback(const GoalHandle::SharedPtr & goal_handle)
{
  if (!goal_handle) {
    RCLCPP_ERROR(node_.get_logger(), "Goal rechazado por el servidor");
    result_promise_.set_value(rclcpp_action::ResultCode::UNKNOWN);
    return;
  }
  RCLCPP_INFO(node_.get_logger(), "Goal aceptado, esperando resultado...");
}

void NavigateToPoseCpp::feedback_callback(
  const GoalHandle::SharedPtr & goal_handle,
  const std::shared_ptr<const NavigateToPose::Feedback> & feedback)
{
  // navigation_time lo mide Nav2 con su reloj (tiempo simulado si use_sim_time).
  const rclcpp::Duration elapsed(feedback->navigation_time);
  RCLCPP_INFO_THROTTLE(
    node_.get_logger(), *node_.get_clock(), 2000,
    "Tiempo de navegación: %.1f s, restante: %.1f s",
    elapsed.seconds(), rclcpp::Duration(feedback->estimated_time_remaining).seconds());

  if (!cancel_requested_ && elapsed > nav_timeout_) {
    RCLCPP_WARN(node_.get_logger(), "La navegación tarda demasiado, cancelando goal");
    cancel_requested_ = true;
    // En C++ el goal handle no tiene cancel(); se cancela a través del cliente.
    action_client_->async_cancel_goal(
      goal_handle, [this](const auto &) {
        RCLCPP_INFO(node_.get_logger(), "Cancelación solicitada");
      });
  }
}

void NavigateToPoseCpp::result_callback(const GoalHandle::WrappedResult & result)
{
  switch (result.code) {
    case rclcpp_action::ResultCode::SUCCEEDED:
      RCLCPP_INFO(node_.get_logger(), "Navegación completada");
      break;
    case rclcpp_action::ResultCode::ABORTED:
      RCLCPP_ERROR(node_.get_logger(), "Navegación abortada");
      break;
    case rclcpp_action::ResultCode::CANCELED:
      RCLCPP_WARN(node_.get_logger(), "Navegación cancelada");
      break;
    default:
      RCLCPP_ERROR(node_.get_logger(), "Código de resultado desconocido");
      break;
  }
  result_promise_.set_value(result.code);
}

}  // namespace fuga_cpp_navigate_to_pose
