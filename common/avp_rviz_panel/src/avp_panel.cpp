#include "avp_panel.hpp"

#include <pluginlib/class_list_macros.hpp>
#include <QTimer>
#include <QMetaObject>
#include <thread>

namespace avp_rviz_panel
{

AVPPanel::AVPPanel(QWidget *parent)
: rviz_common::Panel(parent)
{
  // Initialize ROS node
  ros_node_ = std::make_shared<rclcpp::Node>("avp_panel_node");

  // Publisher to send AVP start command
  publisher_ = ros_node_->create_publisher<std_msgs::msg::String>("/avp/command", 10);

  // Subscriber for AVP status updates
  status_subscriber_ = ros_node_->create_subscription<std_msgs::msg::String>(
    "/avp/status", 10,
    [this](const std_msgs::msg::String::SharedPtr msg)
    {
      // Update the label safely in the Qt main thread
      QString text = QString::fromStdString("Status: " + msg->data);
      QMetaObject::invokeMethod(
        this,
        [this, text]() {
          status_label_->setText(text);
        },
        Qt::QueuedConnection);
    });

  spots_subscriber_ = ros_node_->create_subscription<std_msgs::msg::String>(
  "/parking_spots/empty", 10,
  [this](const std_msgs::msg::String::SharedPtr msg)
  {
    std::string raw = msg->data;
    std::string::size_type colon = raw.rfind(':');
    std::string just_spots = (colon != std::string::npos) ? raw.substr(colon + 1) : raw;
    QString text = QString::fromStdString("Available Spots:" + just_spots);

    QMetaObject::invokeMethod(
      this,
      [this, text]() {
        parking_spots_label_->setText(text);
      },
      Qt::QueuedConnection);
  });


  // Subscriber for AVP spot info
  spot_subscriber_ = ros_node_->create_subscription<std_msgs::msg::String>(
    "/avp/spot", 10,
    [](const std_msgs::msg::String::SharedPtr msg)
    {
      RCLCPP_INFO(rclcpp::get_logger("AVPPanel"), "Received spot info: %s", msg->data.c_str());
    });

  // GUI setup
  auto *layout = new QVBoxLayout;
  start_button_ = new QPushButton("Start AVP");
  status_label_ = new QLabel("Status: Idle");
  parking_spots_label_ = new QLabel("Available Spots: N/A");
  layout->insertWidget(0, parking_spots_label_); 
  layout->addWidget(start_button_);
  layout->addWidget(status_label_);
  setLayout(layout);

  connect(start_button_, &QPushButton::clicked, this, &AVPPanel::onStartButtonClicked);

  // Spin the node in a separate thread
  executor_ = std::make_shared<rclcpp::executors::SingleThreadedExecutor>();
  executor_->add_node(ros_node_);
  std::thread([this]() {
    executor_->spin();
  }).detach();
}

void AVPPanel::onStartButtonClicked()
{
  std_msgs::msg::String msg;
  msg.data = "start";
  publisher_->publish(msg);
  // Remove this line so the label updates only through subscription:
  // status_label_->setText("Dropping off passenger...");
}

}  // namespace avp_rviz_panel

PLUGINLIB_EXPORT_CLASS(avp_rviz_panel::AVPPanel, rviz_common::Panel)
