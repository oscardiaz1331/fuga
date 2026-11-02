#include "fuga_cpp_navigate_to_pose/navigate_to_pose.hpp"

namespace fuga_cpp_navigate_to_pose
{

NavigateToPoseCpp::NavigateToPoseCpp(rclcpp::Node & node)
: node_(node)
{
  action_client_ = rclcpp_action::create_client<NavigateToPose>(&node_, "navigate_to_pose");
}

bool NavigateToPoseCpp::cancel_goal() const
{
  if (!current_goal_handle_) {
    RCLCPP_WARN(node_.get_logger(), "No hay goal activo para cancelar");
    return false;
  }
  action_client_->async_cancel_goal(current_goal_handle_, [this](const auto &) {
    RCLCPP_INFO(node_.get_logger(), "Cancelación solicitada");
  });
  return true;
}

NavigateToPoseCpp::ResultFuture NavigateToPoseCpp::send_goal(
  const geometry_msgs::msg::PoseStamped & pose)
{
  // Promesa propia de este goal; las lambdas de abajo la capturan (ver ResultPromise)
  auto promise = std::make_shared<std::promise<rclcpp_action::ResultCode>>();
  ResultFuture future = promise->get_future().share();

  if (!action_client_->wait_for_action_server(server_timeout_)) {
    RCLCPP_ERROR(
      node_.get_logger(), "Servidor de acción 'navigate_to_pose' no disponible tras %ld s",
      static_cast<long>(server_timeout_.count()));
    promise->set_value(rclcpp_action::ResultCode::UNKNOWN);
    return future;
  }

  NavigateToPose::Goal goal{};
  goal.pose = pose;
  cancel_requested_ = false;

  rclcpp_action::Client<NavigateToPose>::SendGoalOptions options{};
  options.goal_response_callback = [this, promise](const GoalHandle::SharedPtr & goal_handle) {
    goal_response_callback(goal_handle, promise);
  };
  options.feedback_callback = [this, promise](
                                const GoalHandle::SharedPtr & goal_handle,
                                const std::shared_ptr<const NavigateToPose::Feedback> & feedback) {
    feedback_callback(goal_handle, feedback);
  };
  options.result_callback = [this, promise](const GoalHandle::WrappedResult & result) {
    result_callback(result, promise);
  };

  action_client_->async_send_goal(goal, options);
  return future;
}

void NavigateToPoseCpp::goal_response_callback(
  const GoalHandle::SharedPtr & goal_handle, const ResultPromise & promise)
{
  if (!goal_handle) {
    RCLCPP_ERROR(node_.get_logger(), "Goal rechazado por el servidor");
    promise->set_value(rclcpp_action::ResultCode::UNKNOWN);
    return;
  }
  current_goal_handle_ = goal_handle;
  RCLCPP_INFO(node_.get_logger(), "Goal aceptado, esperando resultado...");
}

void NavigateToPoseCpp::feedback_callback(
  const GoalHandle::SharedPtr & goal_handle,
  const std::shared_ptr<const NavigateToPose::Feedback> & feedback)
{
  // navigation_time lo mide Nav2 con su reloj (tiempo simulado si use_sim_time).
  const rclcpp::Duration elapsed(feedback->navigation_time);
  RCLCPP_INFO_THROTTLE(
    node_.get_logger(), *node_.get_clock(), 2000, "Tiempo de navegación: %.1f s, restante: %.1f s",
    elapsed.seconds(), rclcpp::Duration(feedback->estimated_time_remaining).seconds());

  if (!cancel_requested_ && elapsed > nav_timeout_) {
    RCLCPP_WARN(node_.get_logger(), "La navegación tarda demasiado, cancelando goal");
    cancel_requested_ = true;
    // En C++ el goal handle no tiene cancel(); se cancela a través del cliente.
    action_client_->async_cancel_goal(goal_handle, [this](const auto &) {
      RCLCPP_INFO(node_.get_logger(), "Cancelación solicitada");
    });
  }
}

void NavigateToPoseCpp::result_callback(
  const GoalHandle::WrappedResult & result, const ResultPromise & promise)
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
  promise->set_value(result.code);
  // El cliente olvida el goal al recibir su resultado, y cancelarlo después lanzaría
  // UnknownGoalHandleError: por eso se borra el handle. Solo si es el goal actual: el
  // resultado de un goal anterior (abortado por Nav2 al aceptar uno nuevo) puede llegar
  // después de aceptarse el nuevo, y no debe borrarlo.
  if (current_goal_handle_ && current_goal_handle_->get_goal_id() == result.goal_id) {
    current_goal_handle_.reset();
  }
}

}  // namespace fuga_cpp_navigate_to_pose
