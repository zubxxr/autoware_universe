// avp_panel.cpp
#include "avp_panel.hpp"
#include <pluginlib/class_list_macros.hpp>
#include <QTimer>
#include <std_msgs/msg/int32.hpp>
#include <QGroupBox>
#include <QFormLayout>
#include <QSpacerItem>

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

  // === Vehicle Info Group ===
  auto *vehicle_info_group = new QGroupBox("Vehicle Info");
  auto *vehicle_info_layout = new QFormLayout;
  vehicle_count_label_ = new QLabel("...");
  queue_label_ = new QLabel("...");
  vehicle_info_layout->addRow("<b>Vehicles Active:<b>", vehicle_count_label_);
  vehicle_info_layout->addRow("<b>Queue:<b>", queue_label_);
  vehicle_info_group->setLayout(vehicle_info_layout);
  main_layout->addWidget(vehicle_info_group);

  // === Parking Info Group ===
  auto *parking_info_group = new QGroupBox("Parking Info");
  auto *parking_info_layout = new QFormLayout;
  available_spots_label_ = new QLabel("[]");
  reserved_spots_label_ = new QLabel("[]");
  parking_info_layout->addRow("<b>Available Spots:<b>", available_spots_label_);
  parking_info_layout->addRow("<b>Reserved Spots:<b>", reserved_spots_label_);
  parking_info_group->setLayout(parking_info_layout);
  main_layout->addWidget(parking_info_group);

  // === Status Group ===
  auto *status_group = new QGroupBox("System Status");
  auto *status_layout = new QFormLayout;
  status_label_ = new QLabel("<b>Waiting...<b>");
  status_layout->addRow("<b>Status:<b>", status_label_);
  status_group->setLayout(status_layout);
  main_layout->addWidget(status_group);

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

  available_spots_label_->setTextFormat(Qt::RichText);
  reserved_spots_label_->setTextFormat(Qt::RichText);
  queue_label_->setTextFormat(Qt::RichText);
  status_label_->setTextFormat(Qt::RichText);
  vehicle_count_label_->setTextFormat(Qt::RichText);

  // Default state: Drop-off active
  setStyle(dropoff_button_, false);
  setStyle(parking_button_, false);
  setStyle(retrieve_button_, false);

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
      QString text = QString::fromStdString("<b>" + just_spots + "</b>");
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
      QString text = QString::fromStdString("<b>" + just_spots + "</b>");
      QMetaObject::invokeMethod(this, [this, text]() {
        reserved_spots_label_->setText(text);
      }, Qt::QueuedConnection);
    });

  queue_sub_ = node_->create_subscription<std_msgs::msg::String>(
    "/avp/queue", 10,
    [this](const std_msgs::msg::String::SharedPtr msg) {
      std::string raw = msg->data;
      std::string cleaned = raw;

      const std::string prefix = "Drop-off Queue: ";
      std::size_t pos = raw.find(prefix);
      if (pos != std::string::npos) {
        cleaned = raw.substr(pos + prefix.length());
      }

      QString text = QString::fromStdString("<b>" + cleaned + "</b>");
      QMetaObject::invokeMethod(this, [this, text]() {
        queue_label_->setText(text);
      }, Qt::QueuedConnection);
    });

  status_sub_ = node_->create_subscription<std_msgs::msg::String>(
    "/avp/status", 10,
    [this](const std_msgs::msg::String::SharedPtr msg) {
      QString text = QString::fromStdString("<b>" + msg->data + "</b>");
      QMetaObject::invokeMethod(this, [this, text]() {
        status_label_->setText(text);
      }, Qt::QueuedConnection);
    });

  vehicle_count_sub_ = node_->create_subscription<std_msgs::msg::Int32>(
    "/avp/vehicle_count", 10,
    [this](const std_msgs::msg::Int32::SharedPtr msg) {
      QString text = QString("<b>%1</b>").arg(msg->data);
      QMetaObject::invokeMethod(this, [this, text]() {
        vehicle_count_label_->setText(text);
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
