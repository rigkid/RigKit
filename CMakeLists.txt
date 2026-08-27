cmake_minimum_required(VERSION 3.19)

# Host SemVer - one source of truth: cmake/VERSION (see docs/versioning.md).
# Not repo-root VERSION: ${RIGKIT_ROOT} is a public include, and Windows would
# treat that file as C++ <version>.
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/VERSION" RIGKIT_VERSION LIMIT_COUNT 1)
string(STRIP "${RIGKIT_VERSION}" RIGKIT_VERSION)
if(NOT RIGKIT_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
	message(FATAL_ERROR "cmake/VERSION must be MAJOR.MINOR.PATCH (got '${RIGKIT_VERSION}')")
endif()
project(rigkit VERSION ${RIGKIT_VERSION} LANGUAGES C CXX)

# Windows resource compiler for the app icon (add_rigkit_application embeds a
# generated .rc). Must be enabled at file scope, not inside the function.
if(WIN32)
	enable_language(RC)
endif()

# RigKit source root (this directory). Use instead of CMAKE_SOURCE_DIR so
# product apps can add_subdirectory(rigkit) without path breakage.
set(RIGKIT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}" CACHE INTERNAL "RigKit source root")
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	set(RIGKIT_IS_TOP_LEVEL TRUE)
else()
	set(RIGKIT_IS_TOP_LEVEL FALSE)
endif()

# ----------------------------------------------------
# CPM.cmake - lightweight CMake package manager
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

# Native Pi / optional ANGLE to GLES compile path (glad gles2 + ES context).
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
# Library only - GLFW's `docs` target would steal the name RigKit uses for API HTML.
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

# Windows host tool: same AppIcon identicon as runtime, written to a `.ico`
# so add_rigkit_application can embed GLFW_ICON when app.json has no "icon".
if(WIN32 AND NOT TARGET gen-app-icon)
	add_executable(gen-app-icon
		"${RIGKIT_ROOT}/tools/gen-app-icon/main.cpp"
		"${RIGKIT_ROOT}/src/core/util/AppIcon.cpp"
	)
	target_include_directories(gen-app-icon PRIVATE "${RIGKIT_ROOT}/src")
	set_target_properties(gen-app-icon PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tools"
		RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/tools"
		RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/tools"
		RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/tools"
		RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/tools"
	)
endif()

# Host version string - one generated TU so a VERSION bump does not cascade.
configure_file(
	${RIGKIT_ROOT}/cmake/RigKitVersion.cpp.in
	${CMAKE_CURRENT_BINARY_DIR}/RigKitVersion.cpp
	@ONLY
)
target_sources(rigkit PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/RigKitVersion.cpp)

# MinGW PE objects default to a ~32k section limit. EnTT-heavy TUs exceed it
# and the linker then reports nonsense undefined refs into libstdc++/tuple.
# MSYS2 UCRT64 often leaves the MINGW CMake var unset - key off GNU-on-Windows.
if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	target_compile_options(rigkit PUBLIC -Wa,-mbig-obj)
endif()

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

	# Seed for the default identicon (must match runtime AppSettings.appName).
	set(_app_icon_seed "${APP_FOLDER_NAME}")
	if(EXISTS ${APP_MANIFEST})
		file(READ "${APP_MANIFEST}" _manifest)
		string(JSON _project_name GET "${_manifest}" name)
		set(_app_icon_seed "${_project_name}")

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
	# CPM included under add_subdirectory(rigkit) is not visible there - re-include
	# so optional-pack CPMAddPackage works (PaintPlotter / templates/app).
	include("${RIGKIT_ROOT}/cmake/CPM.cmake")
	rigkit_load_packs(${APP_MANIFEST} APP_PACK_LIBS)
	message(STATUS "[RigKit] APP_PACK_LIBS after rigkit_load_packs: ${APP_PACK_LIBS}")

	file(GLOB APP_SRC
		"${APP_DIR}/*.cpp"
		"${APP_DIR}/*.c"
	)

	# Optional app icon from app.json: "icon": "<file>.ico" (path relative to
	# the app folder). Windows always embeds a PE GLFW_ICON (custom file, or
	# gen-app-icon identicon from the app name). All platforms: custom icons
	# are deployed next to the exe for glfwSetWindowIcon at runtime.
	set(_app_icon "")
	if(EXISTS ${APP_MANIFEST})
		rigkit_json_get_string("${_manifest}" _app_icon icon)
	endif()
	set(_app_icon_path "")
	if(NOT _app_icon STREQUAL "")
		set(_app_icon_path "${APP_DIR}/${_app_icon}")
		if(NOT EXISTS "${_app_icon_path}")
			message(WARNING "[RigKit] app.json 'icon' not found: ${_app_icon_path}")
			set(_app_icon_path "")
		endif()
	endif()
	# Windows PE icon (GLFW_ICON): custom app.json "icon", or the default
	# identicon from gen-app-icon so Explorer keeps the mark when the app is closed.
	set(_app_icon_rc "")
	if(WIN32)
		set(_app_icon_dir "${CMAKE_CURRENT_BINARY_DIR}/rigkit_app_icon")
		file(MAKE_DIRECTORY "${_app_icon_dir}")
		file(WRITE "${_app_icon_dir}/app_icon.rc" "GLFW_ICON ICON \"app.ico\"\n")
		set(_app_icon_rc "${_app_icon_dir}/app_icon.rc")
		if(NOT _app_icon_path STREQUAL "")
			configure_file("${_app_icon_path}" "${_app_icon_dir}/app.ico" COPYONLY)
		elseif(TARGET gen-app-icon)
			add_custom_command(
				OUTPUT "${_app_icon_dir}/app.ico"
				COMMAND gen-app-icon "${_app_icon_seed}" "${_app_icon_dir}/app.ico"
				DEPENDS gen-app-icon
				COMMENT "Generate default PE icon for ${_sanitized_name}"
				VERBATIM
			)
			set_source_files_properties("${_app_icon_rc}" PROPERTIES
				OBJECT_DEPENDS "${_app_icon_dir}/app.ico"
			)
			add_custom_target(${_sanitized_name}_app_icon DEPENDS "${_app_icon_dir}/app.ico")
		endif()
	endif()

	add_executable(${_sanitized_name} ${APP_SRC} ${_app_icon_rc})
	if(TARGET ${_sanitized_name}_app_icon)
		add_dependencies(${_sanitized_name} ${_sanitized_name}_app_icon)
	endif()
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

	# Deploy runtime data next to the binary: <exeDir>/data/ (AppPaths).
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

	# Icon next to the exe at the app.json-relative path - the runtime resolves
	# "icon" against the exe dir (window / taskbar icon on Linux and Windows).
	if(NOT _app_icon_path STREQUAL "")
		get_filename_component(_app_icon_reldir "${_app_icon}" DIRECTORY)
		add_custom_command(TARGET ${_sanitized_name} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E make_directory
					"$<TARGET_FILE_DIR:${_sanitized_name}>/${_app_icon_reldir}"
			COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_app_icon_path}"
					"$<TARGET_FILE_DIR:${_sanitized_name}>/${_app_icon}"
			COMMENT "Deploy icon next to ${_sanitized_name}"
		)
	endif()

	# Ship each linked pack's pack.json for About / IPack identity at runtime.
	# Pack themes/ folders merge into <exeDir>/data/themes (color-scheme JSON).
	get_property(_rigkit_processed_packs GLOBAL PROPERTY RIGKIT_PROCESSED_PACKS)
	if(NOT _rigkit_processed_packs)
		set(_rigkit_processed_packs "${APP_PACK_LIBS}")
	endif()
	foreach(pack_lib ${_rigkit_processed_packs})
		set(_pack_json "${RIGKIT_ROOT}/packs/${pack_lib}/pack.json")
		if(EXISTS "${_pack_json}")
			add_custom_command(TARGET ${_sanitized_name} POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E make_directory
						"$<TARGET_FILE_DIR:${_sanitized_name}>/data/packs/${pack_lib}"
				COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_pack_json}"
						"$<TARGET_FILE_DIR:${_sanitized_name}>/data/packs/${pack_lib}/pack.json"
				COMMENT "Deploy ${pack_lib}/pack.json next to ${_sanitized_name}"
			)
		endif()
		set(_pack_themes "${RIGKIT_ROOT}/packs/${pack_lib}/themes")
		if(EXISTS "${_pack_themes}")
			add_custom_command(TARGET ${_sanitized_name} POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E make_directory
						"$<TARGET_FILE_DIR:${_sanitized_name}>/data/themes"
				COMMAND ${CMAKE_COMMAND} -E copy_directory "${_pack_themes}"
						"$<TARGET_FILE_DIR:${_sanitized_name}>/data/themes"
				COMMENT "Deploy ${pack_lib} themes next to ${_sanitized_name}"
			)
		endif()
	endforeach()

	# Packs can declare runtime files (DLLs, path shims) that must sit next to
	# the exe - set the RIGKIT_RUNTIME_FILES property on the pack target.
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

	# Optional API HTML (Doxygen). Target: docs to <build>/docs/api/html/index.html
	include(${RIGKIT_ROOT}/cmake/RigKitDocs.cmake)
endif()
