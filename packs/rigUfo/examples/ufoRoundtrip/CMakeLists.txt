cmake_minimum_required(VERSION 3.19)

get_filename_component(_RIGKIT_EXAMPLE_NAME "${CMAKE_CURRENT_SOURCE_DIR}" NAME)
project(${_RIGKIT_EXAMPLE_NAME} VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(RIGKIT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../.." CACHE PATH "Path to RigKit checkout")
if(NOT EXISTS "${RIGKIT_DIR}/CMakeLists.txt")
	message(FATAL_ERROR "RigKit not found at RIGKIT_DIR=${RIGKIT_DIR}")
endif()

add_subdirectory("${RIGKIT_DIR}" "${CMAKE_BINARY_DIR}/rigkit")
add_rigkit_application(SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
