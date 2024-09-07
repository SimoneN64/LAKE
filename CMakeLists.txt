cmake_minimum_required(VERSION 3.20)

project(lake)

option(BUILD_SHARED_LIBS OFF)

add_subdirectory(external/imgui imgui)
add_subdirectory(external/fmt fmt)
add_subdirectory(external/SDL SDL)
add_subdirectory(external/nfd nfd)

add_executable(lake src/main.cpp src/Window.cpp
  src/LogicAnalyzerView.hpp
  src/LogicAnalyzerView.cpp)

target_include_directories(lake PRIVATE src external/imgui external/imgui/backends)
target_link_libraries(lake PRIVATE fmt::fmt SDL3::SDL3 SDL3::SDL3-static imgui nfd)
set_property(TARGET lake PROPERTY CXX_STANDARD 20)
