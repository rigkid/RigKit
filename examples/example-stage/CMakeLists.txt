cmake_minimum_required(VERSION 3.19)

# In-tree example - nest RigKit from the repo root, build only this app.
get_filename_component(_RIGKIT_EXAMPLE_NAME "${CMAKE_CURRENT_SOURCE_DIR}" NAME)
project(${_RIGKIT_EXAMPLE_NAME} VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(RIGKIT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." CACHE PATH "Path to RigKit checkout")
if(NOT EXISTS "${RIGKIT_DIR}/CMakeLists.txt")
	message(FATAL_ERROR "RigKit not found at RIGKIT_DIR=${RIGKIT_DIR}")
endif()

add_subdirectory("${RIGKIT_DIR}" "${CMAKE_BINARY_DIR}/rigkit")
add_rigkit_application(SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_definitions(example-stage PRIVATE STAGE_WITH_EDITOR=1)

# Same sources, no editor packs. This is the distributable play build.
file(GLOB STAGE_APP_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_executable(example-stage-play ${STAGE_APP_SRC})
target_compile_definitions(example-stage-play PRIVATE STAGE_WITH_EDITOR=0)
target_link_libraries(example-stage-play PRIVATE
	rigkit
	rigComponent
	rigSystems
	rigRender3D
	rigSimComponent
	rigPhysics
	rigJolt
	rigAssimp
	rigPython
)
target_include_directories(example-stage-play PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}"
	"${RIGKIT_DIR}"
	"${RIGKIT_DIR}/src"
	"${RIGKIT_DIR}/packs/rigComponent/src"
	"${RIGKIT_DIR}/packs/rigSystems/src"
	"${RIGKIT_DIR}/packs/rigRender3D/src"
	"${RIGKIT_DIR}/packs/rigSimComponent/src"
	"${RIGKIT_DIR}/packs/rigPhysics/src"
	"${RIGKIT_DIR}/packs/rigJolt/src"
	"${RIGKIT_DIR}/packs/rigAssimp/src"
	"${RIGKIT_DIR}/packs/rigPython/src"
)
set_target_properties(example-stage-play PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
	RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin"
	RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin"
	RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/bin"
	RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/bin"
)
add_custom_command(TARGET example-stage-play POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_CURRENT_SOURCE_DIR}/data"
		"$<TARGET_FILE_DIR:example-stage-play>/data"
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${CMAKE_CURRENT_SOURCE_DIR}/app.json"
		"$<TARGET_FILE_DIR:example-stage-play>/app.json"
	COMMENT "Deploy Stage data next to example-stage-play"
)
if(TARGET rigPython)
	get_target_property(_stage_py_runtime rigPython RIGKIT_RUNTIME_FILES)
	if(_stage_py_runtime)
		add_custom_command(TARGET example-stage-play POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
				${_stage_py_runtime}
				"$<TARGET_FILE_DIR:example-stage-play>"
			COMMENT "Deploy Python runtime next to example-stage-play"
		)
	endif()
endif()

enable_testing()
if(NOT TARGET doctest)
	add_subdirectory("${RIGKIT_DIR}/third_party/doctest" "${CMAKE_BINARY_DIR}/doctest")
endif()
add_executable(example-stage-tests
	tests/test_stage.cpp
	StageScene.cpp
)
target_link_libraries(example-stage-tests PRIVATE
	doctest::doctest
	rigkit
	rigComponent
	rigSimComponent
)
target_include_directories(example-stage-tests PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}"
	"${RIGKIT_DIR}"
	"${RIGKIT_DIR}/src"
	"${RIGKIT_DIR}/packs/rigComponent/src"
	"${RIGKIT_DIR}/packs/rigSimComponent/src"
	"${RIGKIT_DIR}/third_party/doctest"
)
set_target_properties(example-stage-tests PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
	RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin"
	RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin"
	RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/bin"
	RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/bin"
)
add_test(NAME example-stage-tests COMMAND example-stage-tests)
add_test(NAME example-stage-smoke COMMAND "$<TARGET_FILE:example-stage>" --smoke)
set_tests_properties(example-stage-smoke PROPERTIES
	WORKING_DIRECTORY "$<TARGET_FILE_DIR:example-stage>"
)
