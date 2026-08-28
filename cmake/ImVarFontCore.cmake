# Shared ImVarFont core (FreeType layout). One static lib so rigPdf and
# rigLayout do not both compile varfont_core.cpp into the same exe.
#
# GL fulfillment is a separate lib so interactive hosts can use the real
# coverage shaders (varfont_gl) while PDF-only consumers keep the no-op stub:
#   imvarfont_core     — Face / measure / DrawString (calls glr::*)
#   imvarfont_gl_stub  — no-op GPU seam (PDF emit)
#   imvarfont_gl       — analytic coverage shaders (preview / zoom)
#
# IMVARFONT_HOST_GL=ON (set by Relayout before add_subdirectory) attaches the
# real GL lib to core. Default attaches the stub so headless PDF still links.

if(NOT TARGET imvarfont_core)
	set(IMVARFONT_ROOT "$ENV{IMVARFONT_ROOT}" CACHE PATH "Path to ImVarFont checkout")
	if(NOT IMVARFONT_ROOT OR NOT EXISTS "${IMVARFONT_ROOT}/varfont_core.cpp")
		if(DEFINED RIGKIT_ROOT AND EXISTS "${RIGKIT_ROOT}/../ImVarFont/varfont_core.cpp")
			set(IMVARFONT_ROOT "${RIGKIT_ROOT}/../ImVarFont" CACHE PATH "Path to ImVarFont checkout" FORCE)
		elseif(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../../ImVarFont/varfont_core.cpp")
			set(IMVARFONT_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../ImVarFont" CACHE PATH "Path to ImVarFont checkout" FORCE)
		endif()
	endif()

	if(NOT EXISTS "${IMVARFONT_ROOT}/varfont_core.cpp")
		message(STATUS "imvarfont_core: ImVarFont not at IMVARFONT_ROOT - measure/shape off")
		return()
	endif()

	find_package(Freetype QUIET)
	if(NOT Freetype_FOUND)
		message(STATUS "imvarfont_core: ImVarFont found but FreeType missing - measure/shape off")
		return()
	endif()

	add_library(imvarfont_core STATIC
		${IMVARFONT_ROOT}/varfont_core.cpp
	)
	target_include_directories(imvarfont_core PUBLIC
		$<BUILD_INTERFACE:${IMVARFONT_ROOT}>
	)
	# Include system FreeType headers for the compile. Do not link Freetype::Freetype
	# - Relayout already links PDF-Writer's bundled FreeType, and two FT copies collide.
	get_target_property(_ft_inc Freetype::Freetype INTERFACE_INCLUDE_DIRECTORIES)
	if(_ft_inc)
		target_include_directories(imvarfont_core PRIVATE ${_ft_inc})
	endif()
	target_compile_definitions(imvarfont_core PUBLIC RIGKIT_HAS_VARFONT=1)

	find_package(PkgConfig QUIET)
	if(PkgConfig_FOUND)
		pkg_check_modules(HARFBUZZ QUIET IMPORTED_TARGET harfbuzz)
		if(HARFBUZZ_FOUND)
			target_compile_definitions(imvarfont_core PUBLIC IMVARFONT_USE_HARFBUZZ)
			target_link_libraries(imvarfont_core PUBLIC PkgConfig::HARFBUZZ)
			message(STATUS "imvarfont_core: HarfBuzz enabled")
		else()
			message(STATUS "imvarfont_core: HarfBuzz not found (kern table only)")
		endif()
	else()
		message(STATUS "imvarfont_core: pkg-config missing - HarfBuzz off (kern table only)")
	endif()

	message(STATUS "imvarfont_core: ${IMVARFONT_ROOT}")
endif()

if(NOT TARGET imvarfont_core)
	return()
endif()

set(_stub "")
if(DEFINED RIGKIT_ROOT AND EXISTS "${RIGKIT_ROOT}/packs/rigPdf/src/PdfVarFontGlStub.cpp")
	set(_stub "${RIGKIT_ROOT}/packs/rigPdf/src/PdfVarFontGlStub.cpp")
elseif(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../packs/rigPdf/src/PdfVarFontGlStub.cpp")
	set(_stub "${CMAKE_CURRENT_LIST_DIR}/../packs/rigPdf/src/PdfVarFontGlStub.cpp")
endif()

if(NOT TARGET imvarfont_gl_stub AND NOT _stub STREQUAL "")
	add_library(imvarfont_gl_stub STATIC ${_stub})
	target_include_directories(imvarfont_gl_stub PUBLIC
		$<BUILD_INTERFACE:${IMVARFONT_ROOT}>
	)
endif()

if(NOT TARGET imvarfont_gl AND EXISTS "${IMVARFONT_ROOT}/varfont_gl.cpp")
	find_package(OpenGL QUIET)
	if(OpenGL_FOUND)
		add_library(imvarfont_gl STATIC
			${IMVARFONT_ROOT}/varfont_gl.cpp
		)
		target_include_directories(imvarfont_gl PUBLIC
			$<BUILD_INTERFACE:${IMVARFONT_ROOT}>
		)
		target_link_libraries(imvarfont_gl PUBLIC OpenGL::GL)
		if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm|aarch64" OR DEFINED ENV{RIGKIT_FORCE_GLES})
			target_compile_definitions(imvarfont_gl PRIVATE IMVARFONT_GLES=1)
		endif()
		message(STATUS "imvarfont_gl: ${IMVARFONT_ROOT}/varfont_gl.cpp")
	else()
		message(STATUS "imvarfont_gl: OpenGL not found - GPU coverage off")
	endif()
endif()

# Attach exactly one GL seam to core so the final exe never links stub + gl.
get_property(_imvarfont_gl_attached GLOBAL PROPERTY IMVARFONT_GL_ATTACHED)
if(NOT _imvarfont_gl_attached)
	if(IMVARFONT_HOST_GL AND TARGET imvarfont_gl)
		target_link_libraries(imvarfont_core PUBLIC imvarfont_gl)
		set_property(GLOBAL PROPERTY IMVARFONT_GL_ATTACHED gl)
		message(STATUS "imvarfont_core: GPU coverage via imvarfont_gl")
	elseif(TARGET imvarfont_gl_stub)
		target_link_libraries(imvarfont_core PUBLIC imvarfont_gl_stub)
		set_property(GLOBAL PROPERTY IMVARFONT_GL_ATTACHED stub)
		message(STATUS "imvarfont_core: GPU seam stubbed (PDF / headless)")
	else()
		message(STATUS "imvarfont_core: no GL seam - DrawString fill will no-op")
	endif()
endif()
