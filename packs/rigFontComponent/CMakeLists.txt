file(READ "${CMAKE_CURRENT_LIST_DIR}/pack.json" PACK_JSON)
string(JSON PACK_NAME GET "${PACK_JSON}" name)

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
	src/rigFontComponent.cpp
	src/rigFontComponent.h
	src/FontSerializers.cpp
	src/FontSerializers.h
	src/FontFind.cpp
	src/FontFind.h
	src/CFontFace.h
	src/CFontLayer.h
	src/CFontGlyph.h
	src/CFontComponent.h
	src/CFontAnchor.h
	src/CFontKern.h
	src/CFontGroup.h
	src/CFontAxis.h
	src/CFontAvar.h
	src/CFontCell.h
)
target_link_libraries(${PACK_NAME} PUBLIC rigkit)
if(TARGET rigComponent)
	target_link_libraries(${PACK_NAME} PUBLIC rigComponent)
endif()
if(TARGET rigProject)
	target_link_libraries(${PACK_NAME} PUBLIC rigProject)
endif()
target_include_directories(${PACK_NAME} PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src>
)
message(STATUS "Configured pack: ${PACK_NAME} (font source data-only)")
