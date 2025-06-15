// avp_panel.cpp
#include "avp_panel.hpp"
#include <pluginlib/class_list_macros.hpp>
#include <QTimer>

using std::placeholders::_1;

namespace avp_rviz_panel
{
AVPPanel::AVPPanel(QWidget *parent) : rviz_common::Panel(parent)
{
  setupUI();
  createROSInterfaces();
}

void AVPPanel::onInitialize() {}

void AVPPanel::setupUI()
{
  auto *main_layout = new QVBoxLayout;

  available_spots_label_ = new QLabel("Available Spots: []");
  reserved_spots_label_ = new QLabel("Reserved Spots: []");
  queue_label_ = new QLabel("Queue: []");
  status_label_ = new QLabel("Status: Waiting...");

  main_layout->addWidget(available_spots_label_);
  main_layout->addWidget(reserved_spots_label_);
  main_layout->addWidget(queue_label_);
  main_layout->addWidget(status_label_);

  // Buttons
  head_to_dropoff_button_ = new QPushButton("Head to Drop-Off");
  start_avp_button_ = new QPushButton("Start AVP");
  retrieve_button_ = new QPushButton("Retrieve Vehicle");

  // Create horizontal layout for buttons
  auto *button_layout = new QHBoxLayout;
  button_layout->addWidget(head_to_dropoff_button_);
  button_layout->addWidget(start_avp_button_);
  button_layout->addWidget(retrieve_button_);

  // Add button layout to main layout
  main_layout->addLayout(button_layout);

  // Connect buttons to slots
  connect(head_to_dropoff_button_, &QPushButton::clicked, this, &AVPPanel::onHeadToDropOffClicked);
  connect(start_avp_button_, &QPushButton::clicked, this, &AVPPanel::onStartAVPClicked);
  connect(retrieve_button_, &QPushButton::clicked, this, &AVPPanel::onRetrieveClicked);

  setLayout(main_layout);
}

void AVPPanel::createROSInterfaces()
{
  node_ = rclcpp::Node::make_shared("avp_panel_node");
  command_pub_ = node_->create_publisher<std_msgs::msg::String>("/avp/command", 10);

  available_spots_sub_ = node_->create_subscription<std_msgs::msg::String>(
    "/parking_spots/empty", 10,
    [this](const std_msgs::msg::String::SharedPtr msg) {
      std::string raw = msg->data;
      std::string::size_type colon = raw.rfind(':');
      std::string just_spots = (colon != std::string::npos) ? raw.substr(colon + 1) : raw;
      QString text = QString::fromStdString("Available Spots: " + just_spots);
      QMetaObject::invokeMethod(this, [this, text]() {
        available_spots_label_->setText(text);
      }, Qt::QueuedConnection);
    });

  reserved_spots_sub_ = node_->create_subscription<std_msgs::msg::String>(
    "/parking_spots/reserved", 10,
    [this](const std_msgs::msg::String::SharedPtr msg) {
      std::string raw = msg->data;
      std::string::size_type colon = raw.rfind(':');
      std::string just_spots = (colon != std::string::npos) ? raw.substr(colon + 1) : raw;
      QString text = QString::fromStdString("Reserved Spots: " + just_spots);
      QMetaObject::invokeMethod(this, [this, text]() {
        reserved_spots_label_->setText(text);
      }, Qt::QueuedConnection);
    });
  queue_sub_ = node_->create_subscription<std_msgs::msg::String>(
    "/avp/dropoff_queue", 10,
    [this](const std_msgs::msg::String::SharedPtr msg) {
      std::string raw = msg->data;
      std::string cleaned = raw;

      // Remove known prefix if present
      const std::string prefix = "Drop-off Queue: ";
      std::size_t pos = raw.find(prefix);
      if (pos != std::string::npos) {
        cleaned = raw.substr(pos + prefix.length());
      }

      QString text = QString::fromStdString(cleaned);
      QMetaObject::invokeMethod(this, [this, text]() {
        queue_label_->setText("Queue: " + text);
      }, Qt::QueuedConnection);
    });

  status_sub_ = node_->create_subscription<std_msgs::msg::String>(
    "/avp/status", 10,
    [this](const std_msgs::msg::String::SharedPtr msg) {
      QString text = QString::fromStdString("Status: " + msg->data);
      QMetaObject::invokeMethod(this, [this, text]() {
        status_label_->setText(text);
      }, Qt::QueuedConnection);
    });


  executor_ = std::make_shared<rclcpp::executors::SingleThreadedExecutor>();
  executor_->add_node(node_);
  std::thread([this]() {
    executor_->spin();
  }).detach();
}

void AVPPanel::onHeadToDropOffClicked()
{
  std_msgs::msg::String msg;
  msg.data = "head_to_dropoff";
  command_pub_->publish(msg);
}

void AVPPanel::onStartAVPClicked()
{
  std_msgs::msg::String msg;
  msg.data = "start_avp";
  command_pub_->publish(msg);
}

void AVPPanel::onRetrieveClicked()
{
  std_msgs::msg::String msg;
  msg.data = "retrieve";
  command_pub_->publish(msg);
}

}  // namespace avp_rviz_panel

PLUGINLIB_EXPORT_CLASS(avp_rviz_panel::AVPPanel, rviz_common::Panel)
