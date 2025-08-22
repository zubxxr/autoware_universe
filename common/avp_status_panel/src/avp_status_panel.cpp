// avp_status_panel.cpp
#include "avp_status_panel.hpp"
#include <pluginlib/class_list_macros.hpp>
#include <QTimer>
#include <std_msgs/msg/int32.hpp>
#include <QGroupBox>
#include <QFormLayout>
#include <QSpacerItem>
#include "nlohmann/json.hpp"

using std::placeholders::_1;

namespace avp_status_panel
{
AVPStatusPanel::AVPStatusPanel(QWidget *parent) : rviz_common::Panel(parent)
{
  setupUI();
  createROSInterfaces();
}

void AVPStatusPanel::onInitialize() {}

void AVPStatusPanel::setupUI()
{

  auto *main_layout = new QVBoxLayout;

  // === Parking Info Group ===
  auto *parking_info_group = new QGroupBox("Parking Info");
  auto *parking_info_layout = new QFormLayout;
  available_spots_label_ = new QLabel("[]");
  parking_info_layout->addRow("<b>Available Spots:<b>", available_spots_label_);
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
      "  padding: 4px 12px;"
      "  border: none;"
      "  border-radius: 10px;"
      "  background-color: %1;"
      "  color: white;"
      "}"
      "QPushButton:disabled {"
      "  background-color:rgb(12, 13, 14);"     // much darker gray
      "  color: #666;"                   // faded grey text
      "  opacity: 0.4;"                  // further fade the whole thing
      "}"
      "QPushButton:hover:!disabled {"
      "  background-color:rgb(101, 110, 116);"     // on hover
      "}"
      "QPushButton:pressed:!disabled {"
      "  background-color: #74b9ff;"     // lighter blue on press
      "}"
    ).arg(active ? "#0984e3" : "#3b3f47"));  // active = blue, inactive = slate grey
  };

  available_spots_label_->setTextFormat(Qt::RichText);
  status_label_->setTextFormat(Qt::RichText);

  // Default state: Drop-off active
  setStyle(dropoff_button_, false);
  setStyle(parking_button_, false);
  setStyle(retrieve_button_, false);

  avp_mode_layout_->addWidget(dropoff_button_);
  avp_mode_layout_->addWidget(parking_button_);
  avp_mode_layout_->addWidget(retrieve_button_);
  main_layout->addLayout(avp_mode_layout_);


  // Connect buttons to slots
  connect(dropoff_button_, &QPushButton::clicked, this, &AVPStatusPanel::onHeadToDropOffClicked);
  connect(parking_button_, &QPushButton::clicked, this, &AVPStatusPanel::onStartAVPClicked);
  connect(retrieve_button_, &QPushButton::clicked, this, &AVPStatusPanel::onRetrieveClicked);

  setLayout(main_layout);
}

void AVPStatusPanel::createROSInterfaces()
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


  status_sub_ = node_->create_subscription<std_msgs::msg::String>(
    "/avp/status", 10,
    [this](const std_msgs::msg::String::SharedPtr msg) {
      const std::string status_raw = msg->data;
      QString status_text = QString::fromStdString("<b>" + status_raw + "</b>");

      QMetaObject::invokeMethod(this, [this, status_text, status_raw]() {
        status_label_->setText(status_text);
      
        // Logic to enable/disable buttons based on status
        if (status_raw == "Arrived at location.") {
          dropoff_button_->setEnabled(true);
          parking_button_->setEnabled(false);
          retrieve_button_->setEnabled(false);
        } else if (status_raw == "On standby...") {
          dropoff_button_->setEnabled(false);
          parking_button_->setEnabled(true);
          retrieve_button_->setEnabled(false);
        } 
        else if (status_raw == "Autonomous valet parking started...") {
          dropoff_button_->setEnabled(false);
          parking_button_->setEnabled(false);
          retrieve_button_->setEnabled(false);
        }
        else if (status_raw == "Car has been parked.") {
          dropoff_button_->setEnabled(false);
          parking_button_->setEnabled(false);
          retrieve_button_->setEnabled(true);
        } else if (status_raw == "Retrieved.") {
          dropoff_button_->setEnabled(false);
          parking_button_->setEnabled(false);
          retrieve_button_->setEnabled(false);
        }
      }, Qt::QueuedConnection);
    });

  executor_ = std::make_shared<rclcpp::executors::SingleThreadedExecutor>();
  executor_->add_node(node_);
  std::thread([this]() {
    executor_->spin();
  }).detach();
}

void AVPStatusPanel::onHeadToDropOffClicked()
{
  std_msgs::msg::String msg;
  msg.data = "head_to_dropoff";
  command_pub_->publish(msg);
  
  dropoff_button_->setEnabled(false);

}

void AVPStatusPanel::onStartAVPClicked()
{
  std_msgs::msg::String msg;
  msg.data = "start_avp";
  command_pub_->publish(msg);

  parking_button_->setEnabled(false);
}

void AVPStatusPanel::onRetrieveClicked()
{
  std_msgs::msg::String msg;
  msg.data = "retrieve";
  command_pub_->publish(msg);

  retrieve_button_->setEnabled(false);

}

}  // namespace avp_status_panel

PLUGINLIB_EXPORT_CLASS(avp_status_panel::AVPStatusPanel, rviz_common::Panel)
