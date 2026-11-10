cmake_minimum_required(VERSION 3.19)
project(example-esp32 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Native self-test of the ESP32 sample. This is not a GLFW app and it does not
# call add_rigkit_application. The board image is PlatformIO (platformio.ini).

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../../firmware/esp ${CMAKE_BINARY_DIR}/rigesp)

find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
	add_custom_target(example-esp32-pack
		COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/scripts/pack_web.py
		WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
	)
	add_dependencies(rigesp example-esp32-pack)
endif()

add_executable(example-esp32 src/selftest.cpp)
target_link_libraries(example-esp32 PRIVATE rigesp)
set_target_properties(example-esp32 PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

enable_testing()
add_test(NAME example-esp32 COMMAND example-esp32)
