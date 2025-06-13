#pragma once

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include <rviz_common/panel.hpp>

namespace avp_rviz_panel
{

class AVPPanel : public rviz_common::Panel
{
  Q_OBJECT

public:
  explicit AVPPanel(QWidget *parent = nullptr);
  void onInitialize() override;

protected Q_SLOTS:
  void onHeadToDropOffClicked();
  void onStartAVPClicked();
  void onRetrieveClicked();

private:
  void setupUI();
  void createROSInterfaces();

  // UI labels
  QLabel *available_spots_label_;
  QLabel *reserved_spots_label_;
  QLabel *queue_label_;
  QLabel *status_label_;

  // UI buttons
  QPushButton *head_to_dropoff_button_;
  QPushButton *start_avp_button_;
  QPushButton *retrieve_button_;

  // ROS node and interfaces
  rclcpp::Node::SharedPtr node_;
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr available_spots_sub_;
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr reserved_spots_sub_;
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr queue_sub_;
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr status_sub_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr command_pub_;
  rclcpp::executors::SingleThreadedExecutor::SharedPtr executor_;
};

}  // namespace avp_rviz_panel
