#
# CMakeLists.txt for synergy
#
cmake_minimum_required (VERSION 3.1 FATAL_ERROR)
project ("SocketSynergy" C)

set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

include (MacroEnsureOutOfSourceBuild)
include (MacroAddUninstallTarget)
include (MacroGitVersionInfo)
include (MacroCreateConfigFiles)


#
# OPTIONS / BUILD SETTINGS
#

macro_ensure_out_of_source_build ("Do not build SocketSynergy in the source dir!")

get_version_from_git (SocketSynergy 0.0.0)

enable_testing ()

option (DEBUG
	"Add debugging support while building the software"
	OFF)


#
# DEPENDENCIES
#


#
# BUILDING
#

include_directories (include)

if (${DEBUG})
	add_compile_options (-O0 -ggdb3)
endif ()

add_library (synergyShared SHARED src/synergy.c)

set_target_properties (synergyShared
		PROPERTIES OUTPUT_NAME synergy)

add_executable (synergy.d
		src/daemon.c)

add_executable (listendemo
		src/listendemo.c)

target_link_libraries (synergy.d  synergyShared)
target_link_libraries (listendemo synergyShared)


#
# INSTALLING
#

install (TARGETS synergyShared synergy.d
	LIBRARY       DESTINATION lib
	RUNTIME       DESTINATION sbin
	)

install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/sys/socketsynergy.h
	DESTINATION include/sys)

add_uninstall_target ()


#
# PACKAGING
#

set (CPACK_PACKAGE_NAME "SocketSynergy")
set (CPACK_PACKAGE_VERSION ${SocketSynergy_VERSION})
set (CPACK_PACKAGE_VENDOR "OpenFortress.nl")
set (CPACK_PACKAGE_CONTACT "Rick van Rein <rick@openfortress.nl>")
include (PackAllPossible)
include (CPack)

# create_config_files (SocketSynergy)
