// avp_panel.cpp
#include "avp_panel.hpp"
#include <pluginlib/class_list_macros.hpp>
#include <QTimer>
#include <std_msgs/msg/int32.hpp>
#include <QGroupBox>
#include <QFormLayout>
#include <QSpacerItem>
#include "nlohmann/json.hpp"

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
  vehicle_id_label_ = new QLabel("...");
  vehicle_count_label_ = new QLabel("...");
  queue_label_ = new QLabel("...");
  vehicle_info_layout->addRow("<b>Vehicle ID:<b>", vehicle_id_label_);
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

  // === Other Vehicles Status Group ===
  auto *other_status_group = new QGroupBox("Vehicle Statuses");
  auto *other_status_layout = new QVBoxLayout;
  other_status_label_ = new QLabel("<b>Loading...</b>");
  other_status_label_->setTextFormat(Qt::RichText);
  other_status_layout->addWidget(other_status_label_);
  other_status_group->setLayout(other_status_layout);
  main_layout->addWidget(other_status_group);


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

  vehicle_id_sub_ = node_->create_subscription<std_msgs::msg::String>(
    "/avp/vehicle_id", 10,
    [this](const std_msgs::msg::String::SharedPtr msg) {
      current_vehicle_id_ = msg->data;  // <-- store the ID

      QString text = QString("<b>%1</b>").arg(QString::fromStdString(msg->data));
      QMetaObject::invokeMethod(this, [this, text]() {
        vehicle_id_label_->setText(text);
      }, Qt::QueuedConnection);
    });

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
        } else if (status_raw == "Car has been parked.") {
          dropoff_button_->setEnabled(false);
          parking_button_->setEnabled(false);
          retrieve_button_->setEnabled(true);
          // TO DO 
        } else if (status_raw == "Retrieved.") {
          dropoff_button_->setEnabled(false);
          parking_button_->setEnabled(false);
          retrieve_button_->setEnabled(false);
        }
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

  other_status_sub_ = node_->create_subscription<std_msgs::msg::String>(
    "/avp/status/all", 10,
    [this](const std_msgs::msg::String::SharedPtr msg) {
      std::string raw = msg->data;

      QString display_text = "<b>";
      try {
        auto parsed = nlohmann::json::parse(raw);
        for (auto it = parsed.begin(); it != parsed.end(); ++it) {
          if (it.key() != current_vehicle_id_) {
            display_text += QString("%1: %2<br>")
                              .arg(QString::fromStdString(it.key()))
                              .arg(QString::fromStdString(it.value().get<std::string>()));
          }
        }
      } catch (...) {
        display_text += QString::fromStdString(raw);  // fallback to raw
      }
      display_text += "</b>";

      QMetaObject::invokeMethod(this, [this, display_text]() {
        other_status_label_->setText(display_text);
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
  
  dropoff_button_->setEnabled(false);

}

void AVPPanel::onStartAVPClicked()
{
  std_msgs::msg::String msg;
  msg.data = "start_avp";
  command_pub_->publish(msg);

  parking_button_->setEnabled(false);
}

void AVPPanel::onRetrieveClicked()
{
  std_msgs::msg::String msg;
  msg.data = "retrieve";
  command_pub_->publish(msg);

  retrieve_button_->setEnabled(false);

}

}  // namespace avp_rviz_panel

PLUGINLIB_EXPORT_CLASS(avp_rviz_panel::AVPPanel, rviz_common::Panel)