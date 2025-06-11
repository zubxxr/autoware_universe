#pragma once

#include <rviz_common/panel.hpp>
#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>

#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <memory>

namespace avp_rviz_panel
{

class AVPPanel : public rviz_common::Panel
{
  Q_OBJECT

public:
  explicit AVPPanel(QWidget *parent = nullptr);

protected Q_SLOTS:
  void onStartButtonClicked();

private:
  // ROS
  rclcpp::Node::SharedPtr ros_node_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_;
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr status_subscriber_;
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr spot_subscriber_;
  rclcpp::executors::SingleThreadedExecutor::SharedPtr executor_;
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr spots_subscriber_;

  // GUI
  QPushButton *start_button_;
  QLabel *status_label_;
  QLabel *parking_spots_label_;
};

}  // namespace avp_rviz_panel
