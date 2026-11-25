cmake_minimum_required(VERSION 3.19)

get_filename_component(_RIGKIT_EXAMPLE_NAME "${CMAKE_CURRENT_SOURCE_DIR}" NAME)
project(${_RIGKIT_EXAMPLE_NAME} VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(RIGKIT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." CACHE PATH "Path to RigKit checkout")
if(NOT EXISTS "${RIGKIT_DIR}/CMakeLists.txt")
	message(FATAL_ERROR "RigKit not found at RIGKIT_DIR=${RIGKIT_DIR}")
endif()

# rigPdfWrite brings PDF-Writer's bundled FreeType; do not also link the system
# DLL or FT_Done_Face collides at link (same as Relay).
set(RIGIMGUI_SYSTEM_FREETYPE OFF CACHE BOOL
	"example-pixelEditor: FreeType symbols from PDF-Writer" FORCE)

add_subdirectory("${RIGKIT_DIR}" "${CMAKE_BINARY_DIR}/rigkit")
add_rigkit_application(SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/data" AND TARGET example-pixelEditor)
	add_custom_command(TARGET example-pixelEditor POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory
				"${CMAKE_CURRENT_SOURCE_DIR}/data"
				"$<TARGET_FILE_DIR:example-pixelEditor>/data"
		COMMENT "Deploy example-pixelEditor data/ next to example-pixelEditor"
	)
endif()

# ICC catalog (GitBruno/myProfiles) - optional sibling clone or cache path.
set(RIGKIT_MYPROFILES_ROOT "${RIGKIT_DIR}/../myProfiles" CACHE PATH
	"Path to GitBruno/myProfiles checkout for colour ICC deploy")
if(TARGET example-pixelEditor AND EXISTS "${RIGKIT_MYPROFILES_ROOT}/Standard_RGB")
	add_custom_command(TARGET example-pixelEditor POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E make_directory
				"$<TARGET_FILE_DIR:example-pixelEditor>/data/icc/myProfiles"
		COMMAND ${CMAKE_COMMAND} -E copy_directory
				"${RIGKIT_MYPROFILES_ROOT}"
				"$<TARGET_FILE_DIR:example-pixelEditor>/data/icc/myProfiles"
		COMMENT "Deploy myProfiles ICC tree next to example-pixelEditor"
	)
endif()
