cmake_minimum_required (VERSION 3.1)
project (lingo_ap_tracker)

set(CMAKE_BUILD_TYPE Debug)

find_package(wxWidgets COMPONENTS core base)
include(${wxWidgets_USE_FILE})

find_package(PkgConfig)
pkg_check_modules(yaml-cpp yaml-cpp REQUIRED)

include_directories(
  vendor/hkutil
  ${yaml-cpp_INCLUDE_DIRS}
)

add_executable(lingo_ap_tracker
  main.cpp
  tracker_frame.cpp
  tracker_panel.cpp
  game_data.cpp
)
set_property(TARGET lingo_ap_tracker PROPERTY CXX_STANDARD 17)
set_property(TARGET lingo_ap_tracker PROPERTY CXX_STANDARD_REQUIRED ON)
target_link_libraries(lingo_ap_tracker ${wxWidgets_LIBRARIES} ${yaml-cpp_LIBRARIES})
