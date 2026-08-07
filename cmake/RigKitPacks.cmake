# RigKitPacks.cmake
#
# Loads packs as separate STATIC libraries (Island-style module split).
# Never fold pack sources into librigkit - that kills incremental rebuilds.
#
# Usage (prefer add_rigkit_application from the root CMakeLists):
#   add_rigkit_application(SOURCE_DIR ${CMAKE_SOURCE_DIR})
# Or manually:
#   rigkit_load_packs(${APP_MANIFEST} RIGKIT_PACK_LIBS)
#   target_link_libraries(my_app PRIVATE ${RIGKIT_PACK_LIBS} rigkit)
#
# Dependency pin: prefer "ref" (tag / branch / commit). "branch" is a deprecated alias.
# Local packs/<name>/ under RIGKIT_ROOT wins; otherwise CPM clones at ref.
# Requires RIGKIT_ROOT (set by RigKit's CMakeLists).
#
# Requires CMake >= 3.19 for string(JSON ...)
#
# Pack bookkeeping uses GLOBAL properties (reset every configure). Do not use
# CACHE INTERNAL for these — that survives reconfigure, targets do not, and you
# get false "Target missing; rebuilding pack" warnings.

# Drop legacy cache keys from older RigKitPacks (harmless if absent).
unset(RIGKIT_PROCESSED_PACKS CACHE)
unset(RIGKIT_GLOBAL_PACK_INCLUDE_PATHS CACHE)

# Read optional JSON string member; sets OUT_VAR to "" if missing.
function(rigkit_json_get_string JSON_TEXT OUT_VAR)
	set(_path ${ARGN})
	string(JSON _val ERROR_VARIABLE _err GET "${JSON_TEXT}" ${_path})
	if(_err)
		set(${OUT_VAR} "" PARENT_SCOPE)
	else()
		set(${OUT_VAR} "${_val}" PARENT_SCOPE)
	endif()
endfunction()

function(rigkit_load_packs MANIFEST_PATH OUT_VAR)
	if(NOT EXISTS ${MANIFEST_PATH})
		message(FATAL_ERROR "Manifest file not found: ${MANIFEST_PATH}")
	endif()

	if(${CMAKE_VERSION} VERSION_LESS "3.19")
		message(FATAL_ERROR "RigKitPacks.cmake requires CMake 3.19 or newer for JSON parsing, found ${CMAKE_VERSION}.")
	endif()

	message(STATUS "### Running CMake ${CMAKE_VERSION} (${CMAKE_COMMAND})")
	file(READ "${MANIFEST_PATH}" _manifest)

	get_property(_processed GLOBAL PROPERTY RIGKIT_PROCESSED_PACKS)
	if(NOT _processed)
		set(_processed "")
	endif()
	get_property(_pack_includes GLOBAL PROPERTY RIGKIT_GLOBAL_PACK_INCLUDE_PATHS)
	if(NOT _pack_includes)
		set(_pack_includes "")
	endif()

	string(JSON _depsLen ERROR_VARIABLE _depsErr LENGTH "${_manifest}" dependencies)
	if(_depsErr OR _depsLen EQUAL 0)
		message(STATUS "[RigKitPacks] No dependencies listed in manifest ${MANIFEST_PATH}")
		set(${OUT_VAR} "" PARENT_SCOPE)
		return()
	endif()

	message(STATUS "[RigKitPacks] Dependencies length: ${_depsLen}")

	set(_pack_libs "")
	math(EXPR _lastIdx "${_depsLen}-1")
	foreach(idx RANGE 0 ${_lastIdx})
		rigkit_json_get_string("${_manifest}" depName dependencies ${idx} name)
		rigkit_json_get_string("${_manifest}" depUrl dependencies ${idx} url)
		rigkit_json_get_string("${_manifest}" depRef dependencies ${idx} ref)
		rigkit_json_get_string("${_manifest}" depBranch dependencies ${idx} branch)

		message(STATUS "[RigKitPacks] Processing dependency: ${depName}")

		if(depName STREQUAL "")
			message(WARNING "[RigKitPacks] Dependency entry ${idx} missing 'name'; skipping")
			continue()
		endif()

		# Canonical pin: ref, then deprecated branch, then main.
		if(depRef STREQUAL "" AND NOT depBranch STREQUAL "")
			message(STATUS "[RigKitPacks] ${depName}: 'branch' is deprecated; use 'ref' (using \"${depBranch}\")")
			set(depRef "${depBranch}")
		endif()
		if(depRef STREQUAL "")
			set(depRef "main")
		endif()

		if(NOT DEFINED RIGKIT_ROOT OR RIGKIT_ROOT STREQUAL "")
			message(FATAL_ERROR "[RigKitPacks] RIGKIT_ROOT is not set (include RigKit via add_subdirectory)")
		endif()
		set(PACK_DIR "${RIGKIT_ROOT}/packs/${depName}")

		list(FIND _processed ${depName} _found)
		if(NOT _found EQUAL -1)
			if(TARGET ${depName})
				list(APPEND _pack_libs ${depName})
				message(STATUS "[RigKitPacks] Added ${depName} to _pack_libs (already processed)")
				continue()
			endif()
			# Same-configure inconsistency only (should be rare).
			message(WARNING "[RigKitPacks] Target ${depName} missing; rebuilding pack")
			list(REMOVE_ITEM _processed ${depName})
		endif()

		# Prefer local tree (in-org checkout or initialized submodule). Do not
		# auto-checkout over an existing checkout — use tools/update-packs.
		if(EXISTS "${PACK_DIR}/CMakeLists.txt")
			message(STATUS "[RigKitPacks] Using local pack: ${depName} at ${PACK_DIR} (pin metadata ref=${depRef})")
			# Binary dir required when RigKit is nested (pack path is outside host SOURCE_DIR).
			add_subdirectory(${PACK_DIR} "${CMAKE_BINARY_DIR}/packs/${depName}")
			message(STATUS "[RigKitPacks] Added subdirectory for ${depName}")
		else()
			if(depUrl STREQUAL "")
				message(FATAL_ERROR
					"[RigKitPacks] Missing packs/${depName}/ and no 'url' in manifest.\n"
					"  In-org: git submodule update --init --recursive\n"
					"  Optional: clone into packs/${depName}/ or set url+ref for CPM")
			endif()

			# Full SHA pins need a non-shallow fetch.
			set(_shallow TRUE)
			if(depRef MATCHES "^[0-9a-fA-F]+$" AND NOT depRef MATCHES "[^0-9a-fA-F]")
				string(LENGTH "${depRef}" _refLen)
				if(_refLen GREATER_EQUAL 7)
					set(_shallow FALSE)
				endif()
			endif()

			message(STATUS "[RigKitPacks] Cloning pack: ${depName} @ ${depRef} (shallow=${_shallow})")
			# add_rigkit_application() runs in the product/example directory scope.
			# CMake does not export functions defined under add_subdirectory(rigkit)
			# to that parent — include CPM here (caller's scope) before cloning.
			if(NOT COMMAND CPMAddPackage)
				include("${RIGKIT_ROOT}/cmake/CPM.cmake")
			endif()
			CPMAddPackage(
				NAME ${depName}
				GIT_REPOSITORY ${depUrl}
				GIT_TAG ${depRef}
				GIT_SHALLOW ${_shallow}
				SOURCE_DIR ${PACK_DIR}
			)
		endif()

		if(TARGET ${depName})
			message(STATUS "[RigKitPacks] Target ${depName} created successfully")

			set(PACK_JSON_PATH "${PACK_DIR}/pack.json")
			if(EXISTS ${PACK_JSON_PATH})
				file(READ "${PACK_JSON_PATH}" PACK_JSON)
				string(JSON INCLUDE_PATHS_LENGTH ERROR_VARIABLE _incErr LENGTH "${PACK_JSON}" include_paths)
				if(NOT _incErr AND NOT INCLUDE_PATHS_LENGTH EQUAL 0)
					message(STATUS "[RigKitPacks] Applying include paths for ${depName}")
					math(EXPR INCLUDE_PATHS_LAST_IDX "${INCLUDE_PATHS_LENGTH}-1")
					foreach(path_idx RANGE 0 ${INCLUDE_PATHS_LAST_IDX})
						string(JSON INCLUDE_PATH GET "${PACK_JSON}" include_paths ${path_idx})
						set(FULL_INCLUDE_PATH "${PACK_DIR}/${INCLUDE_PATH}")
						target_include_directories(${depName} PUBLIC "${FULL_INCLUDE_PATH}")
						list(APPEND _pack_includes "${FULL_INCLUDE_PATH}")
						message(STATUS "[RigKitPacks] Added include path: ${FULL_INCLUDE_PATH}")
					endforeach()
					list(REMOVE_DUPLICATES _pack_includes)
				endif()
			endif()
		else()
			message(WARNING "[RigKitPacks] Target ${depName} was not created!")
		endif()

		list(APPEND _processed ${depName})
		list(APPEND _pack_libs ${depName})
		message(STATUS "[RigKitPacks] Added ${depName} to _pack_libs (newly processed)")
	endforeach()

	set_property(GLOBAL PROPERTY RIGKIT_PROCESSED_PACKS "${_processed}")
	set_property(GLOBAL PROPERTY RIGKIT_GLOBAL_PACK_INCLUDE_PATHS "${_pack_includes}")
	# Keep the old variable name readable for add_rigkit_application consumers.
	set(RIGKIT_GLOBAL_PACK_INCLUDE_PATHS "${_pack_includes}" PARENT_SCOPE)

	list(REMOVE_DUPLICATES _pack_libs)
	set(${OUT_VAR} ${_pack_libs} PARENT_SCOPE)
	message(STATUS "[RigKitPacks] Returning pack libraries: ${_pack_libs}")
endfunction()
