cmake_minimum_required (VERSION 3.1)
project (lingo_ap_tracker)

set(CMAKE_BUILD_TYPE Debug)

if (MSVC)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
set(CMAKE_WIN32_EXECUTABLE true)
endif(MSVC)

find_package(wxWidgets CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(yaml-cpp REQUIRED)

include_directories(
  vendor/hkutil
  vendor/apclientpp
  vendor/asio/asio/include
  vendor/nlohmann
  vendor/valijson/include
  vendor/websocketpp
  vendor/wswrap/include
  ${yaml-cpp_INCLUDE_DIRS}
  ${OpenSSL_INCLUDE_DIRS}
)

find_path(SYSTEM_INCLUDE_DIR zlib.h)
include_directories(${SYSTEM_INCLUDE_DIR})

link_directories(${openssl_LIBRARY_DIRS})

add_executable(lingo_ap_tracker
  "src/main.cpp"
  "src/tracker_frame.cpp"
  "src/tracker_panel.cpp"
  "src/game_data.cpp"
  "src/area_popup.cpp"
  "src/ap_state.cpp"
  "src/connection_dialog.cpp"
  "src/eye_indicator.cpp"
  "src/tracker_state.cpp"
  "src/tracker_config.cpp"
)
set_property(TARGET lingo_ap_tracker PROPERTY CXX_STANDARD 20)
set_property(TARGET lingo_ap_tracker PROPERTY CXX_STANDARD_REQUIRED ON)
target_link_libraries(lingo_ap_tracker PRIVATE OpenSSL::SSL OpenSSL::Crypto wx::core wx::base yaml-cpp)
