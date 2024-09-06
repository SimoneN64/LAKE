project(imgui)

file(GLOB_RECURSE SOURCES *.cpp)
file(GLOB_RECURSE HEADERS *.hpp)

add_library(imgui ${SOURCES} ${HEADERS})
target_link_libraries(imgui PRIVATE GL)
target_include_directories(imgui PRIVATE . backends ../SDL/include)