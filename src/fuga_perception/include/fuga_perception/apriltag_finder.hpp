#pragma once

#include <fuga_perception/aruco_opencv.hpp>
#include <image_transport/image_transport.hpp>
#include <rclcpp/rclcpp.hpp>
#include <tf2_ros/transform_broadcaster.hpp>

namespace fuga_perception
{
class AprilTagFinder : public rclcpp::Node
{
public:
  AprilTagFinder();

private:
  image_transport::CameraSubscriber camera_subscriber_;
  FugaArucoDetector aruco_detector_;
  tf2_ros::TransformBroadcaster tf_broadcaster_;
};

}  // namespace fuga_perception
