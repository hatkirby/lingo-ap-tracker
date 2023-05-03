cmake_minimum_required (VERSION 3.1)
project (lingo_ap_tracker)

set(CMAKE_BUILD_TYPE Debug)

find_package(wxWidgets COMPONENTS core base)
include(${wxWidgets_USE_FILE})

find_package(OpenSSL)
find_package(yaml-cpp)

include_directories(
  vendor/hkutil
  vendor/apclientpp
  vendor/asio/asio/include
  vendor/nlohmann
  vendor/valijson/include
  vendor/websocketpp
  vendor/wswrap/include
  ${yaml-cpp_INCLUDE_DIRS}
)

link_directories(${openssl_LIBRARY_DIRS})

add_executable(lingo_ap_tracker
  main.cpp
  tracker_frame.cpp
  tracker_panel.cpp
  game_data.cpp
  area_window.cpp
  area_popup.cpp
  ap_state.cpp
  connection_dialog.cpp
  eye_indicator.cpp
  tracker_state.cpp
)
set_property(TARGET lingo_ap_tracker PROPERTY CXX_STANDARD 17)
set_property(TARGET lingo_ap_tracker PROPERTY CXX_STANDARD_REQUIRED ON)
target_link_libraries(lingo_ap_tracker ${wxWidgets_LIBRARIES} ${yaml-cpp_LIBRARIES} ${openssl_LIBRARIES})
