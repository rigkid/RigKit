# Headless doctest smoke for core + spine packs (data layer).
# Added from RigKit top-level CMake when RIGKIT_BUILD_CONTRACT_SMOKE=ON.

set(CONTRACT_SMOKE_MANIFEST "${CMAKE_CURRENT_SOURCE_DIR}/app.json")
rigkit_load_packs("${CONTRACT_SMOKE_MANIFEST}" CONTRACT_SMOKE_PACKS)

add_executable(contract_smoke
	main.cpp
	test_rigComponent.cpp
	test_rigSystems.cpp
	test_rigProject.cpp
	test_purity.cpp
	test_version.cpp
)

target_link_libraries(contract_smoke PRIVATE doctest::doctest rigkit ${CONTRACT_SMOKE_PACKS})
target_include_directories(contract_smoke PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}"
	"${RIGKIT_ROOT}"
	"${RIGKIT_ROOT}/src"
)
target_compile_definitions(contract_smoke PRIVATE
	"RIGKIT_SOURCE_ROOT=\"${RIGKIT_ROOT}\""
)

if(DEFINED RIGKIT_GLOBAL_PACK_INCLUDE_PATHS)
	foreach(include_path ${RIGKIT_GLOBAL_PACK_INCLUDE_PATHS})
		target_include_directories(contract_smoke PRIVATE "${include_path}")
	endforeach()
endif()

foreach(pack_lib ${CONTRACT_SMOKE_PACKS})
	if(TARGET ${pack_lib})
		set(PACK_DIR "${RIGKIT_ROOT}/packs/${pack_lib}")
		set(PACK_JSON_PATH "${PACK_DIR}/pack.json")
		if(EXISTS "${PACK_JSON_PATH}")
			file(READ "${PACK_JSON_PATH}" PACK_JSON)
			string(JSON INCLUDE_PATHS_LENGTH ERROR_VARIABLE _incErr LENGTH "${PACK_JSON}" include_paths)
			if(NOT _incErr AND NOT INCLUDE_PATHS_LENGTH EQUAL 0)
				math(EXPR INCLUDE_PATHS_LAST_IDX "${INCLUDE_PATHS_LENGTH}-1")
				foreach(path_idx RANGE 0 ${INCLUDE_PATHS_LAST_IDX})
					string(JSON INCLUDE_PATH GET "${PACK_JSON}" include_paths ${path_idx})
					target_include_directories(contract_smoke PRIVATE "${PACK_DIR}/${INCLUDE_PATH}")
				endforeach()
			endif()
		endif()
		# Pack public src/ is enough for component headers (rigComponent/rigProject).
		target_include_directories(contract_smoke PRIVATE "${PACK_DIR}/src")
	endif()
endforeach()

set_target_properties(contract_smoke PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
	RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin"
	RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin"
)

add_test(NAME contract_smoke COMMAND contract_smoke)
set_tests_properties(contract_smoke PROPERTIES WORKING_DIRECTORY
	"$<TARGET_FILE_DIR:contract_smoke>")
