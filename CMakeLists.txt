cmake_minimum_required(VERSION 3.20)
project(LauncherOfParticle)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(cmake/add_dependency_folder.cmake)
include(cmake/get_cxx_flags.cmake)

add_subdirectory(extern)
add_subdirectory(lop)
