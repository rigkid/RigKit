cmake_minimum_required(VERSION 3.19)

# Product app starter - nest RigKit, then register this directory as the app.
project(MyApp VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Path to RigKit: submodule (./rigkit), sibling clone (../RigKit), or override -D RIGKIT_DIR=...
set(RIGKIT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/rigkit" CACHE PATH "Path to RigKit checkout")
if(NOT EXISTS "${RIGKIT_DIR}/CMakeLists.txt")
	set(_rigkit_sibling "${CMAKE_CURRENT_SOURCE_DIR}/../RigKit")
	if(EXISTS "${_rigkit_sibling}/CMakeLists.txt")
		set(RIGKIT_DIR "${_rigkit_sibling}" CACHE PATH "Path to RigKit checkout" FORCE)
	endif()
endif()
if(NOT EXISTS "${RIGKIT_DIR}/CMakeLists.txt")
	message(FATAL_ERROR
		"RigKit not found at RIGKIT_DIR=${RIGKIT_DIR}.\n"
		"  git submodule add https://github.com/rigkid/RigKit.git rigkit\n"
		"  git submodule update --init --recursive")
endif()

# CPM in *this* scope: add_rigkit_application() runs here; functions from
# add_subdirectory(rigkit) are not visible to the parent. Newer RigKit also
# re-includes CPM inside add_rigkit_application - this keeps older SHAs working.
include("${RIGKIT_DIR}/cmake/CPM.cmake")

add_subdirectory("${RIGKIT_DIR}" "${CMAKE_BINARY_DIR}/rigkit")

# Same helper used by RigKit's examples/ - app sources live in this directory.
add_rigkit_application(SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
