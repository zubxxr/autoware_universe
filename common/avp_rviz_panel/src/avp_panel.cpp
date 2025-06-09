#include "avp_panel.hpp"

#include <pluginlib/class_list_macros.hpp>

namespace avp_rviz_panel
{

AVPPanel::AVPPanel(QWidget *parent)
: rviz_common::Panel(parent)
{
  // Initialize ROS node
  ros_node_ = std::make_shared<rclcpp::Node>("avp_panel_node");

  // Publisher to send AVP start command
  publisher_ = ros_node_->create_publisher<std_msgs::msg::String>("/avp/command", 10);

  // Subscriber to display status messages
  subscriber_ = ros_node_->create_subscription<std_msgs::msg::String>(
    "/avp/status", 10,
    [this](const std_msgs::msg::String::SharedPtr msg)
    {
      status_label_->setText(QString::fromStdString(msg->data));
    });

  // GUI setup
  auto *layout = new QVBoxLayout;

  start_button_ = new QPushButton("Start AVP");
  status_label_ = new QLabel("Status: Idle");

  layout->addWidget(start_button_);
  layout->addWidget(status_label_);
  setLayout(layout);

  connect(start_button_, &QPushButton::clicked, this, &AVPPanel::onStartButtonClicked);
}

void AVPPanel::onStartButtonClicked()
{
  std_msgs::msg::String msg;
  msg.data = "start";
  publisher_->publish(msg);
  status_label_->setText("Dropping off pedestrian...");
}

}  // namespace avp_rviz_panel

PLUGINLIB_EXPORT_CLASS(avp_rviz_panel::AVPPanel, rviz_common::Panel)
