#pragma once

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include <rviz_common/panel.hpp>
#include <std_msgs/msg/int32.hpp>
#include <QGroupBox>
#include <QFormLayout>
#include <QSpacerItem>

namespace avp_status_panel
{

class AVPStatusPanel : public rviz_common::Panel
{
  Q_OBJECT

public:
  explicit AVPStatusPanel(QWidget *parent = nullptr);
  void onInitialize() override;

protected Q_SLOTS:
  void onHeadToDropOffClicked();
  void onStartAVPClicked();
  void onRetrieveClicked();

private:
  void setupUI();
  void createROSInterfaces();

  std::string current_vehicle_id_;
  
  // UI labels
  QLabel *available_spots_label_;
  QLabel *status_label_;

  QPushButton *dropoff_button_;
  QPushButton *parking_button_;
  QPushButton *retrieve_button_;
  QHBoxLayout *avp_mode_layout_;

  // ROS node and interfaces
  rclcpp::Node::SharedPtr node_;
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr available_spots_sub_;
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr status_sub_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr command_pub_;
  rclcpp::executors::SingleThreadedExecutor::SharedPtr executor_;


};

}  // namespace avp_status_panel