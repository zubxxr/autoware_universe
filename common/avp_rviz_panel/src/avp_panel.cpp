// avp_panel.cpp
#include "avp_panel.hpp"
#include <pluginlib/class_list_macros.hpp>
#include <QTimer>
#include <std_msgs/msg/int32.hpp>

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

  vehicle_count_label_ = new QLabel("Vehicles Active: ...");
  available_spots_label_ = new QLabel("Available Spots: []");
  reserved_spots_label_ = new QLabel("Reserved Spots: []");
  queue_label_ = new QLabel("Queue: []");
  status_label_ = new QLabel("Status: Waiting...");

  avp_mode_layout_ = new QHBoxLayout;

  dropoff_button_ = new QPushButton("Drop-off");
  parking_button_ = new QPushButton("Parking");
  retrieve_button_ = new QPushButton("Retrieve");

  dropoff_button_->setCursor(Qt::PointingHandCursor);
  parking_button_->setCursor(Qt::PointingHandCursor);
  retrieve_button_->setCursor(Qt::PointingHandCursor);

  auto setStyle = [](QPushButton *button, bool active) {
    button->setStyleSheet(QString(
      "QPushButton {"
      "  padding: 4px 12px; "
      "  border: none; "
      "  border-radius: 10px; "
      "  background-color: %1; "
      "  color: white;"
      "}"
      "QPushButton:hover {"
      "  background-color: #0984e3;"  // Light blue on hover
      "}"
      "QPushButton:pressed {"
      "  background-color: #6c5ce7;"  // Purple on click
      "}"
    ).arg(active ? "#00b894" : "#636e72"));
  };

  // Default state: Drop-off active
  setStyle(dropoff_button_, false);
  setStyle(parking_button_, false);
  setStyle(retrieve_button_, false);

  main_layout->addWidget(available_spots_label_);
  main_layout->addWidget(reserved_spots_label_);
  main_layout->addWidget(queue_label_);
  main_layout->addWidget(status_label_);
  main_layout->addWidget(vehicle_count_label_);

  avp_mode_layout_->addWidget(dropoff_button_);
  avp_mode_layout_->addWidget(parking_button_);
  avp_mode_layout_->addWidget(retrieve_button_);
  main_layout->addLayout(avp_mode_layout_);


  // Connect buttons to slots
  connect(dropoff_button_, &QPushButton::clicked, this, &AVPPanel::onHeadToDropOffClicked);
  connect(parking_button_, &QPushButton::clicked, this, &AVPPanel::onStartAVPClicked);
  connect(retrieve_button_, &QPushButton::clicked, this, &AVPPanel::onRetrieveClicked);

  setLayout(main_layout);
}

void AVPPanel::createROSInterfaces()
{
  node_ = rclcpp::Node::make_shared("avp_panel_node");
  command_pub_ = node_->create_publisher<std_msgs::msg::String>("/avp/command", 10);

  available_spots_sub_ = node_->create_subscription<std_msgs::msg::String>(
    "/avp/parking_spots", 10,
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
    "/avp/reserved_parking_spots", 10,
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
    "/avp/queue", 10,
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

  vehicle_count_sub_ = node_->create_subscription<std_msgs::msg::Int32>(
    "/avp/vehicle_count", 10,
    [this](const std_msgs::msg::Int32::SharedPtr msg) {
      QString text = QString("Vehicles Active: %1").arg(msg->data);
      QMetaObject::invokeMethod(this, [this, text]() {
        vehicle_count_label_->setText(text);
      }, Qt::QueuedConnection);
    }
  );


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
