# ADD_UNINSTALL_TARGET()
#    Add custom target 'uninstall' that removes all the files
#    installed by this build (not recommended by CMake devs though).
#
# Add an uninstall target, as described on the CMake wiki.
# Include this file, then call add_uninstall_target().
# Requires a top-level cmake/ directory containing this
# macro file and a cmake_uninstall.cmake.in.

macro(add_uninstall_target)
	# uninstall target
	configure_file(
		"${CMAKE_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
		"${CMAKE_BINARY_DIR}/cmake_uninstall.cmake"
		IMMEDIATE @ONLY)

	add_custom_target(uninstall
		COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/cmake_uninstall.cmake)
endmacro()