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

# Doxygen drops namespace-scope functions unless the header has @file.
# The filter supplies a bare one. check-docs fails if a documented function
# is still missing from the XML.
find_program(RIGKIT_PYTHON NAMES python3 python)
set(RIGKIT_DOXY_FILTER_PATTERNS "")
if(RIGKIT_PYTHON)
	set(_filter_py "${RIGKIT_ROOT}/tools/doxygen_ensure_file.py")
	if(WIN32)
		set(_launcher "${RIGKIT_DOXY_OUT}/ensure-file-filter.cmd")
	else()
		set(_launcher "${RIGKIT_DOXY_OUT}/ensure-file-filter")
	endif()
	file(TO_CMAKE_PATH "${RIGKIT_PYTHON}" _py_fwd)
	file(TO_CMAKE_PATH "${_filter_py}" _py_script)
	file(TO_CMAKE_PATH "${_launcher}" _launcher_fwd)
	if(WIN32)
		file(WRITE "${_launcher}" "@echo off\r\n\"${_py_fwd}\" \"${_py_script}\" \"%~1\"\r\n")
	else()
		file(WRITE "${_launcher}" "#!/bin/sh\nexec \"${_py_fwd}\" \"${_py_script}\" \"\$1\"\n")
		file(CHMOD "${_launcher}" FILE_PERMISSIONS
			OWNER_READ OWNER_WRITE OWNER_EXECUTE
			GROUP_READ GROUP_EXECUTE
			WORLD_READ WORLD_EXECUTE)
	endif()
	set(RIGKIT_DOXY_FILTER_PATTERNS "*.h=\"${_launcher_fwd}\" *.hpp=\"${_launcher_fwd}\"")
else()
	message(STATUS "  Python not found - documented free functions without @file stay out of the HTML")
endif()

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
