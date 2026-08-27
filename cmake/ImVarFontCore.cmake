# Shared ImVarFont core (FreeType layout, no GL). One static lib so rigPdf and
# rigLayout do not both compile varfont_core.cpp into the same exe.

if(TARGET imvarfont_core)
	return()
endif()

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

set(_stub "")
if(DEFINED RIGKIT_ROOT AND EXISTS "${RIGKIT_ROOT}/packs/rigPdf/src/PdfVarFontGlStub.cpp")
	set(_stub "${RIGKIT_ROOT}/packs/rigPdf/src/PdfVarFontGlStub.cpp")
elseif(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../packs/rigPdf/src/PdfVarFontGlStub.cpp")
	set(_stub "${CMAKE_CURRENT_LIST_DIR}/../packs/rigPdf/src/PdfVarFontGlStub.cpp")
endif()
if(_stub STREQUAL "")
	message(STATUS "imvarfont_core: GL stub missing - measure/shape off")
	return()
endif()

add_library(imvarfont_core STATIC
	${IMVARFONT_ROOT}/varfont_core.cpp
	${_stub}
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
