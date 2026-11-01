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

class NavigateToPoseCpp : public rclcpp::Node
{
public:
  using NavigateToPose = nav2_msgs::action::NavigateToPose;
  using GoalHandle = rclcpp_action::ClientGoalHandle<NavigateToPose>;
  using ResultFuture = std::shared_future<rclcpp_action::ResultCode>;

  NavigateToPoseCpp();

  // Envía el goal y devuelve un future que se completa al terminar la navegación.
  // Si el servidor no está disponible o rechaza el goal, se completa con ResultCode::UNKNOWN.
  // Enviar un goal nuevo antes de que termine el anterior rompe el future anterior.
  ResultFuture send_goal(const geometry_msgs::msg::PoseStamped & pose);

private:
  void goal_response_callback(const GoalHandle::SharedPtr & goal_handle);
  void result_callback(const GoalHandle::WrappedResult & result);
  void feedback_callback(
    const GoalHandle::SharedPtr & goal_handle,
    const std::shared_ptr<const NavigateToPose::Feedback> & feedback);

  rclcpp_action::Client<NavigateToPose>::SharedPtr action_client_;
  std::promise<rclcpp_action::ResultCode> result_promise_;
  bool cancel_requested_ = false;

  static constexpr std::chrono::seconds server_timeout_{10};
  static inline const rclcpp::Duration nav_timeout_{std::chrono::seconds(30)};
};

}  // namespace fuga_cpp_navigate_to_pose
