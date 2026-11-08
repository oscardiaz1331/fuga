#include "fuga_perception/apriltag_finder.hpp"

#include <cv_bridge/cv_bridge.hpp>
#include <opencv2/core/mat.hpp>
#include <tf2/LinearMath/Quaternion.hpp>
#include <tf2/LinearMath/Transform.hpp>
#include <tf2/LinearMath/Vector3.hpp>
#include <tf2/transform_datatypes.hpp>

namespace fuga_perception
{
AprilTagFinder::AprilTagFinder()
: rclcpp::Node("apriltag_finder"), aruco_detector_(), tf_broadcaster_(*this)
{
  const float tag_size = declare_parameter("size", 0.2);
  const std::vector<cv::Point3f> tag_corners = {
    {-tag_size / 2.0f, tag_size / 2.0f, 0.0f},
    {tag_size / 2.0f, tag_size / 2.0f, 0.0f},
    {tag_size / 2.0f, -tag_size / 2.0f, 0.0f},
    {-tag_size / 2.0f, -tag_size / 2.0f, 0.0f}};
  camera_subscriber_ = image_transport::create_camera_subscription(
    this, "/camera/image_raw",
    [this, tag_corners](
      const sensor_msgs::msg::Image::ConstSharedPtr & image_msg,
      const sensor_msgs::msg::CameraInfo::ConstSharedPtr & camera_info_msg) {
      const cv::Mat D = cv::Mat(camera_info_msg->d);
      const cv::Mat K = cv::Mat(camera_info_msg->k).reshape(1, 3);
      const auto cv_image = cv_bridge::toCvCopy(image_msg, "bgr8")->image;
      const auto corners = aruco_detector_.detect(cv_image);
      for (const auto & corner : corners) {
        cv::Mat rvec, tvec;
        const bool succeed =
        cv::solvePnP(tag_corners, corner, K, D, rvec, tvec, false, cv::SOLVEPNP_IPPE_SQUARE);
        if (succeed) {
          RCLCPP_INFO(
            this->get_logger(), "Tag detected: rvec = [%f, %f, %f], tvec = [%f, %f, %f]",
            rvec.at<double>(0), rvec.at<double>(1), rvec.at<double>(2), tvec.at<double>(0),
            tvec.at<double>(1), tvec.at<double>(2));
          geometry_msgs::msg::TransformStamped transform_msg;
          transform_msg.header.stamp = image_msg->header.stamp;
          transform_msg.header.frame_id = image_msg->header.frame_id;
          transform_msg.child_frame_id = "exit_tag";
          transform_msg.transform.translation.x = tvec.at<double>(0);
          transform_msg.transform.translation.y = tvec.at<double>(1);
          transform_msg.transform.translation.z = tvec.at<double>(2);
          const tf2::Vector3 r(rvec.at<double>(0), rvec.at<double>(1), rvec.at<double>(2));
          const double angle = r.length();
          // Con ángulo 0 no hay eje: rotación identidad
          const tf2::Quaternion q =
          angle < 1e-9 ? tf2::Quaternion::getIdentity() : tf2::Quaternion(r / angle, angle);
          transform_msg.transform.rotation.x = q.x();
          transform_msg.transform.rotation.y = q.y();
          transform_msg.transform.rotation.z = q.z();
          transform_msg.transform.rotation.w = q.w();
          tf_broadcaster_.sendTransform(transform_msg);
          break;  // Only process the first detected tag for now
        }
      }
    },
    "raw");
}
}  // namespace fuga_perception
