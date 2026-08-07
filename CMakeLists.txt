cmake_minimum_required(VERSION 3.19)

project(rigkit VERSION 1.0.0 LANGUAGES C CXX)

# RigKit source root (this directory). Use instead of CMAKE_SOURCE_DIR so
# product apps can add_subdirectory(rigkit) without path breakage.
set(RIGKIT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}" CACHE INTERNAL "RigKit source root")
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	set(RIGKIT_IS_TOP_LEVEL TRUE)
else()
	set(RIGKIT_IS_TOP_LEVEL FALSE)
endif()

# ----------------------------------------------------
# CPM.cmake – lightweight CMake package manager
# This lets each application fetch only the packs it needs
# ----------------------------------------------------
set(CPM_DOWNLOAD_VERSION "0.38.2")
set(CPM_PATH "${RIGKIT_ROOT}/cmake/CPM.cmake")
if(NOT EXISTS "${CPM_PATH}")
	message(STATUS "Downloading CPM.cmake (v${CPM_DOWNLOAD_VERSION})")
	file(DOWNLOAD
		https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake
		"${CPM_PATH}"
		TLS_VERIFY ON)
endif()
include(${CPM_PATH})

option(BUILD_PACK_EXAMPLES "Build all pack examples" OFF)
# Desktop-only: validate Pi-like GLES via ANGLE. Forbidden on ARM/Pi.
option(RIGKIT_USE_ANGLE "Desktop OpenGL ES via ANGLE (not for Pi)" OFF)
set(RIGKIT_ANGLE_ROOT "" CACHE PATH "Prebuilt ANGLE root (include/ + libGLESv2 + libEGL)")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Native Pi / optional ANGLE → GLES compile path (glad gles2 + ES context).
set(RIGKIT_GLES FALSE)
if(RIGKIT_USE_ANGLE)
	include(${RIGKIT_ROOT}/cmake/RigKitAngle.cmake)
	set(RIGKIT_GLES TRUE)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm" OR CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
	set(RIGKIT_GLES TRUE)
endif()


# Ensure our submodule GLFW is used rather than a system one
set(GLFW3_FOUND OFF CACHE BOOL "" FORCE)
set(GLFW_FOUND OFF CACHE BOOL "" FORCE)
set(GLFW3_USE_STATIC_LIBS ON CACHE BOOL "" FORCE)
# Library only — GLFW's `docs` target would steal the name RigKit uses for API HTML.
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

# Compile units only - headers live on the include path (Island-style: avoid
# listing .h as sources; keeps incremental rebuilds saner).
file(GLOB_RECURSE RIGKIT_SOURCES
	${RIGKIT_ROOT}/src/*.cpp
	${RIGKIT_ROOT}/src/*.c
)
list(FILTER RIGKIT_SOURCES EXCLUDE REGEX "third_party/")

add_subdirectory(${RIGKIT_ROOT}/third_party/glfw ${CMAKE_CURRENT_BINARY_DIR}/third_party/glfw)
add_library(rigkit STATIC ${RIGKIT_SOURCES})
# Packs (rigComponent, rigSystems, rigImGui, ...) stay separate STATIC
# targets via RigKitPacks.cmake - do not fold them into librigkit.

if(RIGKIT_GLES)
	target_sources(rigkit PRIVATE ${RIGKIT_ROOT}/third_party/glad/src/gles2.c)
	target_compile_definitions(rigkit PUBLIC RIGKIT_GLES=1)
	if(RIGKIT_USE_ANGLE)
		target_compile_definitions(rigkit PUBLIC RIGKIT_USE_ANGLE=1)
		rigkit_link_angle(rigkit)
	endif()
else()
	target_sources(rigkit PRIVATE ${RIGKIT_ROOT}/third_party/glad/src/gl.c)
endif()

target_link_libraries(rigkit PRIVATE glfw)

# PUBLIC includes so nested hosts / pack STATIC libs get EnTT, GLM, etc.
# (directory include_directories do not propagate across out-of-tree add_subdirectory).
target_include_directories(rigkit PUBLIC
	${RIGKIT_ROOT}
	${RIGKIT_ROOT}/src
	${RIGKIT_ROOT}/src/core
	${RIGKIT_ROOT}/src/core/pack
	${RIGKIT_ROOT}/src/rendering
	${RIGKIT_ROOT}/src/ecs
	${RIGKIT_ROOT}/src/core/canvas
	${RIGKIT_ROOT}/third_party/glad/include
	${RIGKIT_ROOT}/third_party/glfw/include
	${RIGKIT_ROOT}/third_party/json/single_include
	${RIGKIT_ROOT}/third_party/spdlog/include
	${RIGKIT_ROOT}/third_party/glm
	${RIGKIT_ROOT}/third_party/entt/single_include
)

if(MSVC)
	target_compile_options(rigkit PRIVATE /W4)
else()
	target_compile_options(rigkit PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_custom_target(rigkit_library_built ALL DEPENDS rigkit)

include(${RIGKIT_ROOT}/cmake/RigKitPacks.cmake)

# add_rigkit_application(SOURCE_DIR <dir>)
# <dir> must contain app.json and *.cpp sources. Called from examples/<name>/
# (standalone) and from product apps that nest RigKit.
function(add_rigkit_application)
	cmake_parse_arguments(ARG "" "SOURCE_DIR" "" ${ARGN})
	if(NOT ARG_SOURCE_DIR)
		message(FATAL_ERROR "add_rigkit_application(SOURCE_DIR <path>) requires SOURCE_DIR")
	endif()
	get_filename_component(APP_DIR "${ARG_SOURCE_DIR}" ABSOLUTE)
	get_filename_component(APP_FOLDER_NAME "${APP_DIR}" NAME)
	set(APP_MANIFEST "${APP_DIR}/app.json")

	if(EXISTS ${APP_MANIFEST})
		file(READ "${APP_MANIFEST}" _manifest)
		string(JSON _project_name GET "${_manifest}" name)

		string(REGEX REPLACE "[^a-zA-Z0-9_]" "_" _sanitized_name "${_project_name}")
		string(REGEX REPLACE "_+" "_" _sanitized_name "${_sanitized_name}")
		string(REGEX REPLACE "^_|_$" "" _sanitized_name "${_sanitized_name}")

		if(_sanitized_name STREQUAL "")
			set(_sanitized_name ${APP_FOLDER_NAME})
		endif()

		message(STATUS "[RigKit] Using project name: '${_project_name}' -> '${_sanitized_name}'")
	else()
		set(_sanitized_name ${APP_FOLDER_NAME})
		message(STATUS "[RigKit] No app.json found, using folder name: ${_sanitized_name}")
	endif()

	message(STATUS "[RigKit] Calling rigkit_load_packs for ${_sanitized_name}")
	# Nested hosts: this function runs in the product/example directory scope.
	# CPM included under add_subdirectory(rigkit) is not visible there — re-include
	# so optional-pack CPMAddPackage works (PaintPlotter / templates/app).
	include("${RIGKIT_ROOT}/cmake/CPM.cmake")
	rigkit_load_packs(${APP_MANIFEST} APP_PACK_LIBS)
	message(STATUS "[RigKit] APP_PACK_LIBS after rigkit_load_packs: ${APP_PACK_LIBS}")

	file(GLOB APP_SRC
		"${APP_DIR}/*.cpp"
		"${APP_DIR}/*.c"
	)

	add_executable(${_sanitized_name} ${APP_SRC})
	target_link_libraries(${_sanitized_name} PRIVATE ${APP_PACK_LIBS} rigkit)
	target_include_directories(${_sanitized_name} PRIVATE ${RIGKIT_ROOT} ${APP_DIR})

	# Output: <binaryDir>/bin/<exe> with data/ next to it (no nested bin/<app>/).
	set_target_properties(${_sanitized_name} PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
		RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin"
		RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin"
		RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/bin"
		RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/bin"
	)

	if(DEFINED RIGKIT_GLOBAL_PACK_INCLUDE_PATHS)
		foreach(include_path ${RIGKIT_GLOBAL_PACK_INCLUDE_PATHS})
			target_include_directories(${_sanitized_name} PRIVATE "${include_path}")
		endforeach()
	endif()

	foreach(pack_lib ${APP_PACK_LIBS})
		if(TARGET ${pack_lib})
			set(PACK_DIR "${RIGKIT_ROOT}/packs/${pack_lib}")
			set(PACK_JSON_PATH "${PACK_DIR}/pack.json")

			if(EXISTS ${PACK_JSON_PATH})
				file(READ "${PACK_JSON_PATH}" PACK_JSON)
				string(JSON INCLUDE_PATHS_LENGTH LENGTH "${PACK_JSON}" include_paths)
				if(NOT INCLUDE_PATHS_LENGTH EQUAL 0)
					math(EXPR INCLUDE_PATHS_LAST_IDX "${INCLUDE_PATHS_LENGTH}-1")
					foreach(path_idx RANGE 0 ${INCLUDE_PATHS_LAST_IDX})
						string(JSON INCLUDE_PATH GET "${PACK_JSON}" include_paths ${path_idx})
						target_include_directories(${_sanitized_name} PRIVATE "${PACK_DIR}/${INCLUDE_PATH}")
					endforeach()
				endif()
			else()
				message(WARNING "[RigKit] No pack.json found for ${pack_lib} at ${PACK_JSON_PATH}")
			endif()
		else()
			message(WARNING "[RigKit] Target ${pack_lib} does not exist")
		endif()
	endforeach()

	# Deploy runtime data next to the binary: <exeDir>/data/ (AppPaths, OF bin/data habit).
	# Host assets first, then optional app-local data/ (overlays / extras like .obj).
	set(_rigkit_assets_src "${RIGKIT_ROOT}/assets")
	set(_app_data_src "${APP_DIR}/data")
	if(EXISTS "${_rigkit_assets_src}" OR EXISTS "${_app_data_src}")
		add_custom_command(TARGET ${_sanitized_name} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E make_directory
					"$<TARGET_FILE_DIR:${_sanitized_name}>/data"
			COMMENT "Deploy data/ next to ${_sanitized_name}"
		)
	endif()
	if(EXISTS "${_rigkit_assets_src}")
		add_custom_command(TARGET ${_sanitized_name} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_directory "${_rigkit_assets_src}"
					"$<TARGET_FILE_DIR:${_sanitized_name}>/data"
		)
	endif()
	if(EXISTS "${_app_data_src}")
		add_custom_command(TARGET ${_sanitized_name} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_directory "${_app_data_src}"
					"$<TARGET_FILE_DIR:${_sanitized_name}>/data"
		)
	endif()
	if(EXISTS "${APP_MANIFEST}")
		add_custom_command(TARGET ${_sanitized_name} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different "${APP_MANIFEST}"
					"$<TARGET_FILE_DIR:${_sanitized_name}>/app.json"
			COMMENT "Deploy app.json next to ${_sanitized_name}"
		)
	endif()

	# Packs can declare runtime files (DLLs, path shims) that must sit next to
	# the exe — set the RIGKIT_RUNTIME_FILES property on the pack target.
	foreach(pack_lib ${APP_PACK_LIBS})
		if(TARGET ${pack_lib})
			get_target_property(_pack_runtime_files ${pack_lib} RIGKIT_RUNTIME_FILES)
			if(_pack_runtime_files)
				add_custom_command(TARGET ${_sanitized_name} POST_BUILD
					COMMAND ${CMAKE_COMMAND} -E copy_if_different
							${_pack_runtime_files} "$<TARGET_FILE_DIR:${_sanitized_name}>"
					COMMENT "Deploy ${pack_lib} runtime files next to ${_sanitized_name}"
				)
			endif()
		endif()
	endforeach()

	if(RIGKIT_USE_ANGLE)
		rigkit_deploy_angle_runtime(${_sanitized_name})
	endif()
endfunction()

# Docs / optional tools only when RigKit is the top-level project.
# Example apps are standalone: configure from examples/<name>/ (own build/).
if(RIGKIT_IS_TOP_LEVEL)
	# lifecycle + RigKit-esp32-core host self-test (no ESP-IDF required)
	option(RIGKIT_BUILD_ESP32_CONTRACT_HOST "Build tools/esp32_contract_host host self-test" OFF)
	if(RIGKIT_BUILD_ESP32_CONTRACT_HOST)
		add_subdirectory(${RIGKIT_ROOT}/tools/esp32_contract_host)
	endif()

	# Headless doctest smoke for core + spine packs (data layer).
	option(RIGKIT_BUILD_CONTRACT_SMOKE "Build tools/contract_smoke (doctest + CTest)" ON)
	if(RIGKIT_BUILD_CONTRACT_SMOKE)
		enable_testing()
		add_subdirectory(${RIGKIT_ROOT}/third_party/doctest)
		add_subdirectory(${RIGKIT_ROOT}/tools/contract_smoke)
	endif()

	# Optional API HTML (Doxygen). Target: docs → <build>/docs/api/html/index.html
	include(${RIGKIT_ROOT}/cmake/RigKitDocs.cmake)
endif()
