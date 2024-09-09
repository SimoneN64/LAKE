project(implot)

file(GLOB SOURCES *.cpp)
file(GLOB HEADERS *.h)

add_library(implot ${SOURCES} ${HEADERS})
target_include_directories(implot PRIVATE . ../imgui)
