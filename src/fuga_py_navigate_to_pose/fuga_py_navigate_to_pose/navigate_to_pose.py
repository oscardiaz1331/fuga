from nav2_msgs.action._navigate_to_pose import NavigateToPose_Goal
import rclpy
from rclpy.node import Node
from rclpy.action import ActionClient
from rclpy.task import Future

from nav2_msgs.action import NavigateToPose
from geometry_msgs.msg import PoseStamped
from action_msgs.msg import GoalStatus


class NavigateToPosePython(Node):

    def __init__(self) -> None:
        super().__init__("navigate_to_pose_python")
        self._action_client: ActionClient = ActionClient(self, NavigateToPose, "navigate_to_pose")
        self._goal_handle = None
        self._timeout_s = 30
        self._server_timeout_s = 10.0
        self._cancel_requested = False

    def _feedback_callback(self, msg) -> None:
        feedback: NavigateToPose.Feedback = msg.feedback
        self.get_logger().info(f"Feedback Navigation time:{feedback.navigation_time.sec}s, Remaining time:{feedback.estimated_time_remaining.sec}s")
        if not self._cancel_requested and feedback.navigation_time.sec > self._timeout_s and self._goal_handle is not None:
            self._cancel_requested = True
            self._goal_handle.cancel_goal_async()

    def _goal_response_callback(self, future: Future) -> None:
        goal_handle = future.result()
        if not goal_handle.accepted:
            self.get_logger().error("Goal not accepted")
            return
        self.get_logger().info("Goal accepted, waiting result...")
        self._goal_handle = goal_handle
        result_future = goal_handle.get_result_async()
        result_future.add_done_callback(self._get_result_callback)

    def _get_result_callback(self, future: Future) -> None:
        # future.result() es un GetResult_Response con .status y .result
        wrapped = future.result()
        status = wrapped.status
        result = wrapped.result

        self.get_logger().info(f"Result: {result}")

        if status == GoalStatus.STATUS_SUCCEEDED:
            self.get_logger().info("Navegación completada")
        elif status == GoalStatus.STATUS_ABORTED:
            self.get_logger().error("Navegación abortada")
        elif status == GoalStatus.STATUS_CANCELED:
            self.get_logger().warn("Navegación cancelada")
        else:
            self.get_logger().warn(f"Estado final inesperado: {status}")

    def send_goal(self, pose: PoseStamped) -> None:
        goal: NavigateToPose_Goal = NavigateToPose.Goal()
        goal.pose = pose
        if not self._action_client.wait_for_server(timeout_sec=self._server_timeout_s):
            self.get_logger().error(
                f"Servidor de acción 'navigate_to_pose' no disponible tras {self._server_timeout_s} s")
            return
        send_goal_future = self._action_client.send_goal_async(
            goal, feedback_callback=self._feedback_callback)
        send_goal_future.add_done_callback(self._goal_response_callback)

def main():
    rclpy.init()
    nave2pose_py = NavigateToPosePython()
    pose = PoseStamped()
    pose.header.frame_id = "map"
    pose.header.stamp = nave2pose_py.get_clock().now().to_msg()
    pose.pose.position.x = 0.0
    pose.pose.position.y = 0.0
    nave2pose_py.send_goal(pose)
    rclpy.spin(nave2pose_py)
    rclpy.shutdown()
