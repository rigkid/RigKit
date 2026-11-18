# Run gen-app-icon with retries.
#
# On Windows, ninja often links tools/gen-app-icon.exe and runs it in the same
# -j wave. Defender / the PE loader can still hold the freshly written image,
# and the child dies with 0xC0000005 (access violation) before main() runs.
# The same command succeeds a moment later. Retry with a short sleep.
#
# Expects -DGEN_APP_ICON=... -DAPP_NAME=... -DOUT_ICO=...

if(NOT GEN_APP_ICON OR NOT APP_NAME OR NOT OUT_ICO)
	message(FATAL_ERROR "RunGenAppIcon: need GEN_APP_ICON, APP_NAME, OUT_ICO")
endif()

set(_attempts 8)
set(_delay_s 1)
set(_rc 1)
foreach(_i RANGE 1 ${_attempts})
	execute_process(
		COMMAND "${GEN_APP_ICON}" "${APP_NAME}" "${OUT_ICO}"
		RESULT_VARIABLE _rc
	)
	if(_rc EQUAL 0)
		return()
	endif()
	message(STATUS
		"gen-app-icon failed (attempt ${_i}/${_attempts}, code ${_rc}); retrying in ${_delay_s}s")
	execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep ${_delay_s})
endforeach()

message(FATAL_ERROR
	"gen-app-icon failed after ${_attempts} attempts (last code ${_rc}): ${GEN_APP_ICON} ${APP_NAME} ${OUT_ICO}")
