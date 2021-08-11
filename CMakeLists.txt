cmake_minimum_required(VERSION 3.5)

project(sempr-python VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 14)

find_package(PkgConfig REQUIRED)

pkg_check_modules(sempr REQUIRED sempr-core)
include_directories(${sempr_INCLUDE_DIRS})
link_directories(${sempr_LIBRARY_DIRS})

include_directories(src/external)

find_library(SEMPR_RDF sempr_plugin_rdf HINTS /usr/lib/sempr_plugins)


find_package(pybind11 CONFIG REQUIRED)
include_directories(${pybind11_INCLUDE_DIRS})

pybind11_add_module(
    semprpy
    src/module.cpp
    src/core.cpp
    src/components.cpp
    src/rete.cpp
)

target_link_libraries(semprpy PUBLIC ${sempr_LIBRARIES} ${SEMPR_RDF})
