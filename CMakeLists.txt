cmake_minimum_required(VERSION 3.14)

project(httptunnel LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Network)

add_executable(httptunnel
  sources/main.cpp
  sources/ProcessingService.cpp
  sources/SettingsLoader.cpp
  sources/IncomingHttpTrafficManagement.cpp
  sources/OutgoingHttpTrafficManagement.cpp
  sources/HttpTransceiver.cpp
  sources/Logger.cpp
  include/ProcessingService.hpp
  include/SettingsLoader.hpp
  include/IncomingHttpTrafficManagement.hpp
  include/OutgoingHttpTrafficManagement.hpp
  include/HttpTransceiver.hpp
  include/Logger.hpp
  )

target_link_libraries(httptunnel Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Network)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
				   COMMAND ${CMAKE_COMMAND} -E copy
				   ${CMAKE_SOURCE_DIR}/settings.ini ${CMAKE_BINARY_DIR})
