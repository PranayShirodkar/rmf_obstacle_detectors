#ifndef YOLODETECTOR_HPP
#define YOLODETECTOR_HPP

#include <string>

// ROS includes
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <geometry_msgs/msg/transform.hpp>

// OpenCV includes
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>

// Project includes
#include <rmf_obstacle_msgs/msg/obstacle.hpp>
#include <rmf_obstacle_msgs/msg/obstacles.hpp>

class YoloDetector
{
public:
    using Obstacle = rmf_obstacle_msgs::msg::Obstacle;
    using Obstacles = rmf_obstacle_msgs::msg::Obstacles;

    // User Configurations
    struct Config
    {
        // Camera configurations
        std::string camera_name;
        const bool visualize = true;
        const bool stationary = true;
        const float camera_afov;
        // YoloDetector configurations
        const float score_threshold;
        const float nms_threshold;
        const float confidence_threshold;
    };

    YoloDetector(std::shared_ptr<Config> config);
    ~YoloDetector();
    Obstacles image_cb(const sensor_msgs::msg::Image::ConstSharedPtr &msg);
    void camera_pose_cb(const geometry_msgs::msg::Transform &msg);
    void set_image_detections_pub(rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr image_detections_pub);

private:

    // Yolov5s Constants
    // Camera image gets scaled to this size before inference
    const float INPUT_WIDTH = 640.0;
    const float INPUT_HEIGHT = 640.0;

    // Members
    rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr _image_detections_pub;
    std::vector<std::string> _class_list;
    cv::dnn::Net _net;
    std::shared_ptr<Config> _config;
    float _d_param;
    float _w_param;
    geometry_msgs::msg::Transform _camera_pose;

    // Methods
    cv::Mat format_yolov5(const cv::Mat &source);
    std::vector<cv::Mat> detect(cv::Mat &input_image);
    Obstacles post_process(const cv::Mat &original_image, cv::Mat &image, std::vector<cv::Mat> &detections);
    cv::Point3d img_coord_to_cam_coord(const cv::Point &centroid, const cv::Mat &original_image);
    void cam_coord_to_world_coord(Obstacle &obstacle);
    Obstacles to_rmf_obstacles(const cv::Mat &original_image, const std::vector<int> &final_class_ids, const std::vector<float> &final_confidences, const std::vector<cv::Rect> &final_boxes, const std::vector<cv::Point> &final_centroids);
    void drawing(const cv::Mat &original_image, cv::Mat &image, const std::vector<int> &final_class_ids, const std::vector<float> &final_confidences, const std::vector<cv::Rect> &final_boxes, const std::vector<cv::Point> &final_centroids);
    void draw_label(cv::Mat &input_image, std::string label, int left, int top);
    void calibrate();

};

#endif // YOLODETECTOR_HPP