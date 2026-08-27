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
# Nested pack.json dependencies are loaded depth-first before the pack itself.
# First resolution wins: a pack already in RIGKIT_PROCESSED_PACKS (or an existing
# CMake target of that name) is skipped - app.json order and nested edges may
# both name the same dep without double add_subdirectory.
#
# Requires CMake >= 3.19 for string(JSON ...)
#
# Pack bookkeeping uses GLOBAL properties (reset every configure). Do not use
# CACHE INTERNAL for these - that survives reconfigure, targets do not, and you
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

# Load one pack and its nested pack.json deps. First wins.
# Sets OUT_APPENDED to the pack name when newly (or already) available for linking.
function(rigkit_ensure_pack depName depUrl depRef OUT_APPENDED)
	set(${OUT_APPENDED} "" PARENT_SCOPE)

	if(depName STREQUAL "")
		return()
	endif()

	if(NOT DEFINED RIGKIT_ROOT OR RIGKIT_ROOT STREQUAL "")
		message(FATAL_ERROR "[RigKitPacks] RIGKIT_ROOT is not set (include RigKit via add_subdirectory)")
	endif()

	get_property(_processed GLOBAL PROPERTY RIGKIT_PROCESSED_PACKS)
	if(NOT _processed)
		set(_processed "")
	endif()
	get_property(_pack_includes GLOBAL PROPERTY RIGKIT_GLOBAL_PACK_INCLUDE_PATHS)
	if(NOT _pack_includes)
		set(_pack_includes "")
	endif()

	# First wins: already processed this configure.
	list(FIND _processed ${depName} _found)
	if(NOT _found EQUAL -1)
		if(TARGET ${depName})
			set(${OUT_APPENDED} "${depName}" PARENT_SCOPE)
			message(STATUS "[RigKitPacks] Keep first: ${depName} (already processed)")
			return()
		endif()
		message(WARNING "[RigKitPacks] Target ${depName} missing; rebuilding pack")
		list(REMOVE_ITEM _processed ${depName})
	endif()

	# Target exists (e.g. pack CMake or another loader created it) - keep it.
	if(TARGET ${depName})
		list(APPEND _processed ${depName})
		set_property(GLOBAL PROPERTY RIGKIT_PROCESSED_PACKS "${_processed}")
		set(${OUT_APPENDED} "${depName}" PARENT_SCOPE)
		message(STATUS "[RigKitPacks] Keep first: ${depName} (target already exists)")
		return()
	endif()

	# Canonical pin: ref, then deprecated branch alias handled by caller, then main.
	if(depRef STREQUAL "")
		set(depRef "main")
	endif()

	set(PACK_DIR "${RIGKIT_ROOT}/packs/${depName}")

	# Ensure sources exist before reading nested pack.json / add_subdirectory.
	if(NOT EXISTS "${PACK_DIR}/CMakeLists.txt")
		if(depUrl STREQUAL "")
			message(FATAL_ERROR
				"[RigKitPacks] Missing packs/${depName}/ and no 'url' in manifest.\n"
				"  In-org: git submodule update --init --recursive\n"
				"  Optional: clone into packs/${depName}/ or set url+ref for CPM")
		endif()

		set(_shallow TRUE)
		if(depRef MATCHES "^[0-9a-fA-F]+$" AND NOT depRef MATCHES "[^0-9a-fA-F]")
			string(LENGTH "${depRef}" _refLen)
			if(_refLen GREATER_EQUAL 7)
				set(_shallow FALSE)
			endif()
		endif()

		message(STATUS "[RigKitPacks] Cloning pack: ${depName} @ ${depRef} (shallow=${_shallow})")
		if(NOT COMMAND CPMAddPackage)
			include("${RIGKIT_ROOT}/cmake/CPM.cmake")
		endif()
		# Download only - nested deps must load before this pack's add_subdirectory.
		CPMAddPackage(
			NAME ${depName}
			GIT_REPOSITORY ${depUrl}
			GIT_TAG ${depRef}
			GIT_SHALLOW ${_shallow}
			SOURCE_DIR ${PACK_DIR}
			DOWNLOAD_ONLY YES
		)
	endif()

	if(NOT EXISTS "${PACK_DIR}/CMakeLists.txt")
		message(FATAL_ERROR "[RigKitPacks] Pack ${depName} has no CMakeLists.txt at ${PACK_DIR}")
	endif()

	# Mark processed before recursion / add so cycles keep the first visit.
	list(APPEND _processed ${depName})
	set_property(GLOBAL PROPERTY RIGKIT_PROCESSED_PACKS "${_processed}")

	# Nested pack.json dependencies - depth-first, first wins.
	set(PACK_JSON_PATH "${PACK_DIR}/pack.json")
	if(EXISTS "${PACK_JSON_PATH}")
		file(READ "${PACK_JSON_PATH}" _packJson)
		string(JSON _nestLen ERROR_VARIABLE _nestErr LENGTH "${_packJson}" dependencies)
		if(NOT _nestErr AND NOT _nestLen EQUAL 0)
			math(EXPR _nestLast "${_nestLen}-1")
			foreach(_nidx RANGE 0 ${_nestLast})
				rigkit_json_get_string("${_packJson}" _nName dependencies ${_nidx} name)
				rigkit_json_get_string("${_packJson}" _nUrl dependencies ${_nidx} url)
				rigkit_json_get_string("${_packJson}" _nRef dependencies ${_nidx} ref)
				rigkit_json_get_string("${_packJson}" _nBranch dependencies ${_nidx} branch)
				if(_nRef STREQUAL "" AND NOT _nBranch STREQUAL "")
					message(STATUS "[RigKitPacks] ${_nName}: 'branch' is deprecated; use 'ref' (using \"${_nBranch}\")")
					set(_nRef "${_nBranch}")
				endif()
				if(NOT _nName STREQUAL "")
					message(STATUS "[RigKitPacks] Nested dep of ${depName}: ${_nName}")
					rigkit_ensure_pack("${_nName}" "${_nUrl}" "${_nRef}" _nestedAppended)
					# Nested libs are linked by the pack's own CMake / PUBLIC deps;
					# they are also recorded when the app lists them explicitly.
				endif()
			endforeach()
		endif()
	endif()

	# Re-check: a nested dep must not have created us; skip double add.
	if(TARGET ${depName})
		set(${OUT_APPENDED} "${depName}" PARENT_SCOPE)
		message(STATUS "[RigKitPacks] Keep first: ${depName} (created while loading nested deps)")
		return()
	endif()

	message(STATUS "[RigKitPacks] Using local pack: ${depName} at ${PACK_DIR} (pin metadata ref=${depRef})")
	add_subdirectory(${PACK_DIR} "${CMAKE_BINARY_DIR}/packs/${depName}")
	message(STATUS "[RigKitPacks] Added subdirectory for ${depName}")

	if(TARGET ${depName})
		message(STATUS "[RigKitPacks] Target ${depName} created successfully")
		if(EXISTS "${PACK_JSON_PATH}")
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
				set_property(GLOBAL PROPERTY RIGKIT_GLOBAL_PACK_INCLUDE_PATHS "${_pack_includes}")
			endif()
		endif()
		set(${OUT_APPENDED} "${depName}" PARENT_SCOPE)
	else()
		message(WARNING "[RigKitPacks] Target ${depName} was not created!")
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

	# Nested ensure_pack shares GLOBAL bookkeeping; do not clear mid-configure.
	get_property(_pack_includes GLOBAL PROPERTY RIGKIT_GLOBAL_PACK_INCLUDE_PATHS)
	if(NOT _pack_includes)
		set_property(GLOBAL PROPERTY RIGKIT_GLOBAL_PACK_INCLUDE_PATHS "")
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

		if(depRef STREQUAL "" AND NOT depBranch STREQUAL "")
			message(STATUS "[RigKitPacks] ${depName}: 'branch' is deprecated; use 'ref' (using \"${depBranch}\")")
			set(depRef "${depBranch}")
		endif()
		if(depRef STREQUAL "")
			set(depRef "main")
		endif()

		rigkit_ensure_pack("${depName}" "${depUrl}" "${depRef}" _appended)
		if(NOT _appended STREQUAL "")
			list(APPEND _pack_libs ${_appended})
			message(STATUS "[RigKitPacks] Added ${_appended} to _pack_libs")
		endif()
	endforeach()

	get_property(_pack_includes GLOBAL PROPERTY RIGKIT_GLOBAL_PACK_INCLUDE_PATHS)
	set(RIGKIT_GLOBAL_PACK_INCLUDE_PATHS "${_pack_includes}" PARENT_SCOPE)

	list(REMOVE_DUPLICATES _pack_libs)
	set(${OUT_VAR} ${_pack_libs} PARENT_SCOPE)
	message(STATUS "[RigKitPacks] Returning pack libraries: ${_pack_libs}")
endfunction()
