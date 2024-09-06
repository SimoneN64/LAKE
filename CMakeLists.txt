# CMakeList.txt: progetto CMake per Saleae Logic KLine Extractor. Includere l'origine e definire
# qui la logica specifica del progetto.
#
cmake_minimum_required (VERSION 3.20)

project ("Saleae Logic KLine Extractor")

find_package(fmt CONFIG REQUIRED)

# Aggiungere l'origine all'eseguibile di questo progetto.
add_executable (CMakeTarget main.cpp)

target_link_libraries(CMakeTarget PRIVATE fmt::fmt)
set_property(TARGET CMakeTarget PROPERTY CXX_STANDARD 20)

# TODO: Aggiungere i test e, se necessario, installare le destinazioni.
