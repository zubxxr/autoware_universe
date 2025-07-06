cmake_minimum_required(VERSION 3.14)
project(avp_rviz_panel)

find_package(autoware_cmake REQUIRED)
autoware_package()

find_package(Qt5 REQUIRED COMPONENTS Widgets)
set(QT_LIBRARIES Qt5::Widgets)

# Enable automatic MOC, UIC, and RCC for Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
add_definitions(-DQT_NO_KEYWORDS)

# Declare the plugin source and headers
set(HEADERS
  src/avp_panel.hpp
)

ament_auto_add_library(${PROJECT_NAME} SHARED
  src/avp_panel.cpp
  ${HEADERS}
)

target_link_libraries(${PROJECT_NAME}
  ${QT_LIBRARIES}
)

target_include_directories(${PROJECT_NAME}
  PUBLIC
    include
)

# Export to RViz
pluginlib_export_plugin_description_file(rviz_common plugins/plugin_description.xml)

# Install plugin folder
ament_auto_package(
  INSTALL_TO_SHARE
  plugins
)

install(FILES plugins/plugin_description.xml
  DESTINATION share/${PROJECT_NAME}
)

