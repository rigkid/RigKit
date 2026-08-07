# Optional desktop ANGLE (OpenGL ES via libGLESv2/libEGL).
# Never enable on Raspberry Pi / arm — native GLES is the product path.
#
#   cmake -S . -B build -DRIGKIT_USE_ANGLE=ON -DRIGKIT_ANGLE_ROOT=C:/path/to/angle
# Or vcpkg toolchain with `angle` installed (unofficial-angle).

if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm" OR CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
	message(FATAL_ERROR
		"RIGKIT_USE_ANGLE=ON is not allowed on ARM/aarch64 (Pi). "
		"Use native GLES there (RIGKIT_USE_ANGLE=OFF).")
endif()

set(_angle_root "${RIGKIT_ANGLE_ROOT}")
if(_angle_root STREQUAL "" AND DEFINED ENV{RIGKIT_ANGLE_ROOT})
	set(_angle_root "$ENV{RIGKIT_ANGLE_ROOT}")
endif()

set(RIGKIT_ANGLE_VIA_VCPKG FALSE)
set(_angle_found FALSE)

if(NOT _angle_root STREQUAL "")
	find_path(RIGKIT_ANGLE_INCLUDE_DIR
		NAMES GLES2/gl2.h
		PATHS "${_angle_root}/include" "${_angle_root}"
		NO_DEFAULT_PATH
	)
	find_library(RIGKIT_ANGLE_GLESV2
		NAMES libGLESv2 GLESv2
		PATHS
			"${_angle_root}/lib"
			"${_angle_root}/out/Release"
			"${_angle_root}/out/Release/lib"
			"${_angle_root}/bin"
			"${_angle_root}"
		NO_DEFAULT_PATH
	)
	find_library(RIGKIT_ANGLE_EGL
		NAMES libEGL EGL
		PATHS
			"${_angle_root}/lib"
			"${_angle_root}/out/Release"
			"${_angle_root}/out/Release/lib"
			"${_angle_root}/bin"
			"${_angle_root}"
		NO_DEFAULT_PATH
	)
	if(RIGKIT_ANGLE_INCLUDE_DIR AND RIGKIT_ANGLE_GLESV2 AND RIGKIT_ANGLE_EGL)
		set(_angle_found TRUE)
		set(RIGKIT_ANGLE_ROOT_RESOLVED "${_angle_root}" CACHE INTERNAL "")
		message(STATUS "[RigKitAngle] Using ANGLE from RIGKIT_ANGLE_ROOT=${_angle_root}")
	endif()
endif()

if(NOT _angle_found)
	find_package(unofficial-angle CONFIG QUIET)
	if(TARGET unofficial::angle::libGLESv2 AND TARGET unofficial::angle::libEGL)
		set(_angle_found TRUE)
		set(RIGKIT_ANGLE_VIA_VCPKG TRUE)
		message(STATUS "[RigKitAngle] Using vcpkg unofficial-angle targets")
	endif()
endif()

if(NOT _angle_found)
	message(FATAL_ERROR
		"RIGKIT_USE_ANGLE=ON but ANGLE was not found.\n"
		"  Set -DRIGKIT_ANGLE_ROOT=<dir> with include/ + libGLESv2 + libEGL,\n"
		"  or use a vcpkg toolchain that provides unofficial-angle.\n"
		"  Pi builds must leave RIGKIT_USE_ANGLE=OFF.")
endif()

function(rigkit_link_angle target_name)
	if(RIGKIT_ANGLE_VIA_VCPKG)
		target_link_libraries(${target_name} PUBLIC
			unofficial::angle::libGLESv2
			unofficial::angle::libEGL)
	else()
		target_include_directories(${target_name} PUBLIC "${RIGKIT_ANGLE_INCLUDE_DIR}")
		target_link_libraries(${target_name} PUBLIC
			"${RIGKIT_ANGLE_GLESV2}"
			"${RIGKIT_ANGLE_EGL}")
	endif()
endfunction()

function(rigkit_deploy_angle_runtime app_target)
	if(NOT WIN32 OR RIGKIT_ANGLE_VIA_VCPKG)
		return()
	endif()
	if(NOT RIGKIT_ANGLE_ROOT_RESOLVED)
		return()
	endif()
	set(_root "${RIGKIT_ANGLE_ROOT_RESOLVED}")
	foreach(_name libGLESv2.dll libEGL.dll)
		foreach(_dir "${_root}" "${_root}/bin" "${_root}/lib" "${_root}/out/Release")
			if(EXISTS "${_dir}/${_name}")
				add_custom_command(TARGET ${app_target} POST_BUILD
					COMMAND ${CMAKE_COMMAND} -E copy_if_different
						"${_dir}/${_name}" "$<TARGET_FILE_DIR:${app_target}>"
					COMMENT "Deploy ANGLE ${_name}")
				break()
			endif()
		endforeach()
	endforeach()
endfunction()

message(STATUS "[RigKitAngle] Desktop GLES via ANGLE enabled")
