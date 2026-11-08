#include "fuga_perception/aruco_opencv.hpp"

using namespace fuga_perception;

FugaArucoDetector::FugaArucoDetector()
: dictionary_(cv::aruco::getPredefinedDictionary(cv::aruco::DICT_APRILTAG_36h11)),
  params_(cv::aruco::DetectorParameters::create())
{
}

std::vector<std::vector<cv::Point2f>> FugaArucoDetector::detect(const cv::Mat & image)
{
  std::vector<std::vector<cv::Point2f>> corners;
  std::vector<int> ids;
  cv::aruco::detectMarkers(image, dictionary_, corners, ids, params_);
  return corners;
}
