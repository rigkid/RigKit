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

add_library(${PACK_NAME} STATIC
	src/rigFont.cpp
	src/rigFont.h
	src/FontShape.cpp
	src/FontShape.h
)
target_link_libraries(${PACK_NAME} PUBLIC rigkit)
if(TARGET rigFontComponent)
	target_link_libraries(${PACK_NAME} PUBLIC rigFontComponent)
endif()
if(TARGET rigComponent)
	target_link_libraries(${PACK_NAME} PUBLIC rigComponent)
endif()
target_include_directories(${PACK_NAME} PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src>
)
message(STATUS "Configured pack: ${PACK_NAME} (font source shaper)")
