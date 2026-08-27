# Optional API docs (Doxygen). Adds custom target `docs` when Doxygen is found.
# GLFW docs are disabled in the root CMakeLists (GLFW_BUILD_DOCS=OFF) so this
# name stays free. Output: ${CMAKE_BINARY_DIR}/docs/api/html/index.html

find_package(Doxygen QUIET)
if(NOT DOXYGEN_FOUND)
	message(STATUS "Doxygen not found - skip `docs` target (install doxygen to generate API HTML)")
	return()
endif()

find_program(RIGKIT_DOT_EXECUTABLE NAMES dot)
if(RIGKIT_DOT_EXECUTABLE)
	set(RIGKIT_DOXY_HAVE_DOT YES)
else()
	set(RIGKIT_DOXY_HAVE_DOT NO)
endif()

set(RIGKIT_DOXY_OUT "${CMAKE_BINARY_DIR}/docs/api")
set(RIGKIT_DOXYFILE_IN "${RIGKIT_ROOT}/docs/api/Doxyfile.in")
set(RIGKIT_DOXYFILE_OUT "${RIGKIT_DOXY_OUT}/Doxyfile")

file(MAKE_DIRECTORY "${RIGKIT_DOXY_OUT}")
configure_file("${RIGKIT_DOXYFILE_IN}" "${RIGKIT_DOXYFILE_OUT}" @ONLY)

add_custom_target(docs
	COMMAND ${CMAKE_COMMAND} -E make_directory "${RIGKIT_DOXY_OUT}"
	COMMAND "${DOXYGEN_EXECUTABLE}" "${RIGKIT_DOXYFILE_OUT}"
	WORKING_DIRECTORY "${RIGKIT_ROOT}"
	COMMENT "Generating RigKit API docs (Doxygen) to ${RIGKIT_DOXY_OUT}/html"
	VERBATIM
)

message(STATUS "Doxygen ${DOXYGEN_VERSION} - `cmake --build <build> --target docs`")
if(RIGKIT_DOXY_HAVE_DOT STREQUAL "YES")
	message(STATUS "  Graphviz dot found - class diagrams enabled")
else()
	message(STATUS "  Graphviz dot not found - diagrams disabled (optional)")
endif()
