file(READ "${CMAKE_CURRENT_LIST_DIR}/pack.json" PACK_JSON)
string(JSON PACK_NAME GET "${PACK_JSON}" name)

if(NOT TARGET rigFontComponent)
	if(DEFINED RIGKIT_ROOT AND NOT RIGKIT_ROOT STREQUAL "")
		set(_dep "${RIGKIT_ROOT}/packs/rigFontComponent")
	else()
		set(_dep "${CMAKE_SOURCE_DIR}/packs/rigFontComponent")
	endif()
	if(EXISTS "${_dep}/CMakeLists.txt")
		add_subdirectory("${_dep}" "${CMAKE_BINARY_DIR}/packs/rigFontComponent")
	endif()
endif()

if(NOT TARGET rigComponent)
	if(DEFINED RIGKIT_ROOT AND NOT RIGKIT_ROOT STREQUAL "")
		set(_rigComponent_dir "${RIGKIT_ROOT}/packs/rigComponent")
	else()
		set(_rigComponent_dir "${CMAKE_SOURCE_DIR}/packs/rigComponent")
	endif()
	if(EXISTS "${_rigComponent_dir}/CMakeLists.txt")
		add_subdirectory("${_rigComponent_dir}" "${CMAKE_BINARY_DIR}/packs/rigComponent")
	endif()
endif()

if(NOT TARGET rigProject)
	if(DEFINED RIGKIT_ROOT AND NOT RIGKIT_ROOT STREQUAL "")
		set(_rigProject_dir "${RIGKIT_ROOT}/packs/rigProject")
	else()
		set(_rigProject_dir "${CMAKE_SOURCE_DIR}/packs/rigProject")
	endif()
	if(EXISTS "${_rigProject_dir}/CMakeLists.txt")
		add_subdirectory("${_rigProject_dir}" "${CMAKE_BINARY_DIR}/packs/rigProject")
	endif()
endif()

add_library(${PACK_NAME} STATIC
	src/rigUfo.cpp
	src/rigUfo.h
	src/XmlUtil.cpp
	src/XmlUtil.h
	src/Plist.cpp
	src/Plist.h
	src/Glif.cpp
	src/Glif.h
	src/UfoIo.cpp
	src/UfoIo.h
	src/UfoRoundtrip.cpp
	src/UfoRoundtrip.h
)
target_link_libraries(${PACK_NAME} PUBLIC rigkit)
if(TARGET rigFontComponent)
	target_link_libraries(${PACK_NAME} PUBLIC rigFontComponent)
endif()
if(TARGET rigComponent)
	target_link_libraries(${PACK_NAME} PUBLIC rigComponent)
endif()
if(TARGET rigProject)
	target_link_libraries(${PACK_NAME} PUBLIC rigProject)
endif()
if(NOT TARGET miniz)
	include(FetchContent)
	set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
	set(MINIZ_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
	set(MINIZ_BUILD_TESTS OFF CACHE BOOL "" FORCE)
	FetchContent_Declare(
		miniz
		GIT_REPOSITORY https://github.com/richgel999/miniz.git
		GIT_TAG 3.0.2
		GIT_SHALLOW TRUE
		EXCLUDE_FROM_ALL
	)
	FetchContent_MakeAvailable(miniz)
endif()
if(TARGET miniz)
	target_link_libraries(${PACK_NAME} PRIVATE miniz)
endif()
target_include_directories(${PACK_NAME} PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src>
)

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/tools/ufo_smoke.cpp")
	add_executable(ufo_smoke "${CMAKE_CURRENT_LIST_DIR}/tools/ufo_smoke.cpp")
	target_link_libraries(ufo_smoke PRIVATE ${PACK_NAME})
endif()

message(STATUS "Configured pack: ${PACK_NAME} (UFO IO)")
