# CREATE_CONFIG_FILES(<packagename>)
#    Call this macro to generate CMake and pkg-config configuration
#    files from templates found in the top-level source directory.
#
# Most ARPA2-related components write configuration-information
# files and install them. There are two flavors:
#
#   - CMake config-info files (<foo>Config.cmake and <foo>ConfigVersion.cmake)
#   - pkg-config files (<foo>.pc)
#
# The macro create_config_files() simplifies this process
# by using named template files for all three output files.
# Pass a package name (e.g. "Quick-DER") to the macro, and
# the source files (e.g. <file>.in for the files named above
# will be taken from the top-level source directory.
#
# As an (un)special case, the ConfigVersion file may be taken from
# the cmake/ directory, since there is nothing particularly special
# for that file (as opposed to the other files, which need to 
# specify paths, dependencies, and other things).

# Copyright 2017, Adriaan de Groot <groot@kde.org>
#
# Redistribution and use is allowed according to the terms of the two-clause BSD license.
#    https://opensource.org/licenses/BSD-2-Clause
#    SPDX short identifier: BSD-2-Clause

macro (create_config_files _packagename)
	export (PACKAGE ${_packagename})
	# The CMake configuration files are written to different locations
	# depending on the host platform, since different conventions apply.
	if (WIN32 AND NOT CYGWIN)
		set (DEF_INSTALL_CMAKE_DIR CMake)
	else ()
		set (DEF_INSTALL_CMAKE_DIR lib/cmake/${_packagename})
	endif ()
	set (INSTALL_CMAKE_DIR ${DEF_INSTALL_CMAKE_DIR} CACHE PATH
		"Installation directory for CMake files")

	# Calculate include/ relative to the installed place of the config file.
	file (RELATIVE_PATH REL_INCLUDE_DIR "${CMAKE_INSTALL_PREFIX}/${INSTALL_CMAKE_DIR}"
		"${CMAKE_INSTALL_PREFIX}/include")
	set (CONF_INCLUDE_DIRS "\${${_packagename}_CMAKE_DIR}/${REL_INCLUDE_DIR}")
	# Substitute in real values for the placeholders in the .in files,
	# create the files in the build tree, and install them.
	configure_file (${PROJECT_SOURCE_DIR}/${_packagename}Config.cmake.in
		"${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${_packagename}Config.cmake" @ONLY)
	set (_conf_version_filename ${PROJECT_SOURCE_DIR}/${_packagename}ConfigVersion.cmake.in)
	if (NOT EXISTS ${_conf_version_filename})
		# (un)special-case: use the generic version-checking file,
		# assume ${_packagename}_VERSION exists and copy that to
		# the generic version-variable for this file.
		set (_conf_version_filename ${PROJECT_SOURCE_DIR}/cmake/ConfigVersion.cmake.in)
		set (_conf_version ${${_packagename}_VERSION})
	endif ()
	configure_file (${_conf_version_filename}
		"${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${_packagename}ConfigVersion.cmake" @ONLY)
	configure_file (${PROJECT_SOURCE_DIR}/${_packagename}.pc.in
		"${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${_packagename}.pc" @ONLY)

	install (FILES
		"${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${_packagename}Config.cmake"
		"${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${_packagename}ConfigVersion.cmake"
		DESTINATION "${INSTALL_CMAKE_DIR}" COMPONENT dev)
	install (FILES 
	"${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${_packagename}.pc"
		DESTINATION "lib/pkgconfig/")
endmacro ()
