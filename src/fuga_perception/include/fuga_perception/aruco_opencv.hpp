#pragma once

#include <vector>

#include <opencv2/aruco.hpp>
#include <opencv2/core.hpp>

namespace fuga_perception
{
// OpenCV 4.6 (Ubuntu 24.04 / ROS Jazzy) uses the contrib aruco module:
// cv::aruco::ArucoDetector only exists from OpenCV 4.7 onwards.
class FugaArucoDetector
{
public:
  FugaArucoDetector();

  std::vector<std::vector<cv::Point2f>> detect(const cv::Mat & image);

private:
  cv::Ptr<cv::aruco::Dictionary> dictionary_;
  cv::Ptr<cv::aruco::DetectorParameters> params_;
};
}  // namespace fuga_perception
