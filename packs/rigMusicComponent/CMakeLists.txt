file(READ "${CMAKE_CURRENT_LIST_DIR}/pack.json" PACK_JSON)
string(JSON PACK_NAME GET "${PACK_JSON}" name)

if(NOT TARGET rigComponent)
	if(DEFINED RIGKIT_ROOT AND NOT RIGKIT_ROOT STREQUAL "")
		set(_dep_dir "${RIGKIT_ROOT}/packs/rigComponent")
	else()
		set(_dep_dir "${CMAKE_SOURCE_DIR}/packs/rigComponent")
	endif()
	if(EXISTS "${_dep_dir}/CMakeLists.txt")
		add_subdirectory("${_dep_dir}" "${CMAKE_BINARY_DIR}/packs/rigComponent")
	endif()
endif()

add_library(${PACK_NAME} STATIC
	src/rigMusicComponent.cpp
	src/rigMusicComponent.h
	src/CMusicClock.h
	src/CMusicTransport.h
)
target_link_libraries(${PACK_NAME} PUBLIC rigkit)
if(TARGET rigComponent)
	target_link_libraries(${PACK_NAME} PUBLIC rigComponent)
endif()
target_include_directories(${PACK_NAME} PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src>
)
message(STATUS "Configured pack: ${PACK_NAME} (music data-only)")
