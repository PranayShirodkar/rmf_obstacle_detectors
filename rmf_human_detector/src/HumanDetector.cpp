#include <rclcpp/wait_for_message.hpp>
#include <tf2/LinearMath/Quaternion.h>
#include <tf2/LinearMath/Matrix3x3.h>
#include <sensor_msgs/msg/camera_info.hpp>
#include <rmf_human_detector/HumanDetector.hpp>

HumanDetector::HumanDetector() : Node("human_detector"), _data(std::make_shared<Data>())
{
  make_detector();

  _data->_image_detections_pub = this->create_publisher<sensor_msgs::msg::Image>(
  "/" + _data->_camera_name + "/image_detections",
  rclcpp::QoS(10).reliable()
  );
  _data->_detector->set_image_detections_pub(_data->_image_detections_pub);

  _data->_obstacles_pub = this->create_publisher<Obstacles>(
  "/rmf_obstacles",
  rclcpp::QoS(10).reliable()
  );

  const std::string camera_image_topic =  "/" +  _data->_camera_name + "/image_rect";
  _data->_image_sub = this->create_subscription<sensor_msgs::msg::Image>(
  camera_image_topic,
  1,
  [=](const sensor_msgs::msg::Image::ConstSharedPtr &msg)
  {
    // perform detections
    auto rmf_obstacles_msg = _data->_detector->image_cb(msg);

    // populate fields like time stamp, etc
    for (auto &obstacle : rmf_obstacles_msg.obstacles)
    {
      obstacle.header.stamp = this->get_clock()->now();
    }

    // publish rmf_obstacles_msg
    _data->_obstacles_pub->publish(rmf_obstacles_msg);
  });

  const std::string camera_pose_topic = "/" +  _data->_camera_name + "/pose";
  _data->_camera_pose_sub = this->create_subscription<tf2_msgs::msg::TFMessage>(
  camera_pose_topic,
  1,
  [=](const tf2_msgs::msg::TFMessage::ConstSharedPtr &msg)
  {
    for (auto &transformStamped : msg->transforms)
    {
      if (transformStamped.header.frame_id == _data->_camera_parent_name
      && transformStamped.child_frame_id ==  _data->_camera_name)
      {
        _data->_detector->camera_pose_cb(transformStamped.transform);
      }
    }
  });
}

void HumanDetector::make_detector()
{
  // get ros2 params
  _data->_camera_name = this->declare_parameter(
    "camera_name", "camera1");
  _data->_camera_parent_name = this->declare_parameter(
    "camera_parent_name", "sim_world");
  const bool visualize = this->declare_parameter(
    "visualize", true);
  const bool stationary = this->declare_parameter(
    "static", true);
  const float score_threshold = this->declare_parameter(
    "score_threshold", 0.45);
  const float nms_threshold = this->declare_parameter(
    "nms_threshold", 0.45);
  const float confidence_threshold = this->declare_parameter(
    "confidence_threshold", 0.25);

  // get one camera_info msg
  sensor_msgs::msg::CameraInfo camera_info;
  std::shared_ptr<rclcpp::Node> temp_node = std::make_shared<rclcpp::Node>("wait_for_msg_node");
  const std::string camera_info_topic = "/" + _data->_camera_name + "/camera_info";
  rclcpp::wait_for_message(camera_info, temp_node, camera_info_topic);

  // calculate camera fov
  float f_x = camera_info.p[0];
  float fov_x = 2 * atan2( camera_info.width, (2*f_x) );

  std::shared_ptr<YoloDetector::Config> config = std::make_shared<YoloDetector::Config>(
    YoloDetector::Config{
      _data->_camera_name,
      visualize,
      stationary,
      fov_x,
      score_threshold,
      nms_threshold,
      confidence_threshold,
    }
  );

  _data->_detector = std::make_shared<YoloDetector>(config);
}

HumanDetector::~HumanDetector()
{
}

int main(int argc, char **argv)
{
  rclcpp::init(argc, argv);
  std::cout << "Starting HumanDetector node" << std::endl;
  rclcpp::spin(std::make_shared<HumanDetector>());
  rclcpp::shutdown();
  return 0;
}
