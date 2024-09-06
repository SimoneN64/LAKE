project(imgui)

file(GLOB SOURCES *.cpp)
file(GLOB HEADERS *.h)

list(APPEND SOURCES backends/imgui_impl_sdl3.cpp backends/imgui_impl_sdlrenderer3.cpp)

add_library(imgui ${SOURCES} ${HEADERS})
target_link_libraries(imgui PRIVATE GL)
target_include_directories(imgui PRIVATE . backends ../SDL/include)