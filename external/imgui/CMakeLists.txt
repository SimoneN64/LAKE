project(imgui)

file(GLOB SOURCES *.cpp)
file(GLOB HEADERS *.h)

list(APPEND SOURCES backends/imgui_impl_sdl3.cpp backends/imgui_impl_opengl3.cpp)

add_library(imgui ${SOURCES} ${HEADERS})
if (WIN32)
  target_link_libraries(imgui PRIVATE opengl32)
else ()
  target_link_libraries(imgui PRIVATE GL)
endif ()
target_include_directories(imgui PRIVATE . backends ../SDL/include)
