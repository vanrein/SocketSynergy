# GET_VERSION_FROM_GIT(<appname> <default>)
#    Uses git tags to determine a version name and sets <appname>_VERSION 
#    (along with split-out _MAJOR, _MINOR and _PATCHLEVEL variables). If
#    git isn't available, use <default> for version information (which should
#    be a string in the format M.m.p).
#
# Version Information
#
# This assumes you are working from a git checkout that uses tags
# in an orderly fashion (e.g. according to the ARPA2 project best
# practices guide, with version-* tags). It also checks for local
# modifications and uses that to munge the <patchlevel> part of
# the version number.
#
# Produces version numbers <major>.<minor>.<patchlevel>.
#
# To use the macro, provide an app- or package name; this is
# used to fill variables called <app>_VERSION_MAJOR, .. and
# overall <app>_VERSION. If git can't be found or does not Produce
# meaningful output, use the provided default, e.g.:
#
#   get_version_from_git(Quick-DER 0.1.5)
#
# After the macro invocation, Quick-DER_VERSION is set according
# to the git tag or 0.1.5.
#

# Copyright 2017, Adriaan de Groot <groot@kde.org>
#
# Redistribution and use is allowed according to the terms of the two-clause BSD license.
#    https://opensource.org/licenses/BSD-2-Clause
#    SPDX short identifier: BSD-2-Clause

macro(get_version_from_git _appname _default)
	find_package (Git QUIET)

	if (Git_FOUND)
		message("-- Looking for git-versioning information.")
		exec_program (
			${GIT_EXECUTABLE}
			ARGS diff --quiet
			RETURN_VALUE GIT_HAVE_CHANGES
		)

		exec_program (
			${GIT_EXECUTABLE}
			ARGS describe --tags --match 'version-*.*-*'
			OUTPUT_VARIABLE GIT_VERSION_INFO
		)
	else(NOT Git_FOUND)
		message(WARNING "Git not found; git-versioning uses default ${_default}.")
		set(GIT_VERSION_INFO "version-${_default}")
		set(GIT_HAVE_CHANGES 0)
	endif()

	string (
		REGEX REPLACE "^version-([1-9][0-9]*|0)[.]([1-9][0-9]*|0)-(.*)$"
		"\\1"
		GIT_VERSION_MAJOR
		${GIT_VERSION_INFO}
	)

	string (
		REGEX REPLACE "^version-([1-9][0-9]*|0)[.]([1-9][0-9]*|0)-(.*)$"
		"\\2"
		GIT_VERSION_MINOR
		${GIT_VERSION_INFO}
	)

	if (GIT_HAVE_CHANGES EQUAL 0)
		string (
			REGEX REPLACE "^version-([1-9][0-9]*|0)[.]([1-9][0-9]*|0)-(.*)$"
			"\\3"
			GIT_VERSION_PATCHLEVEL
			${GIT_VERSION_INFO}
		)

		set (
			USER_SUPPLIED_PATCHLEVEL
			"${GIT_VERSION_PATCHLEVEL}"
			CACHE STRING "User-override for patch level under ${GIT_VERSION_MAJOR}.${GIT_VERSION_MINOR}"
		)

	else()

		exec_program (
			date
			ARGS '+%Y%m%d-%H%M%S'
			OUTPUT_VARIABLE GIT_CHANGES_TIMESTAMP
		)
		set (GIT_VERSION_PATCHLEVEL "local-${GIT_CHANGES_TIMESTAMP}")
		message ("  Git reports local changes, fixing patch level to local-${GIT_CHANGES_TIMESTAMP}")

		unset (USER_SUPPLIED_PATCHLEVEL CACHE)

	endif()

	set(${_appname}_VERSION_MAJOR ${GIT_VERSION_MAJOR})
	set(${_appname}_VERSION_MINOR ${GIT_VERSION_MINOR})
	set(${_appname}_VERSION_PATCHLEVEL ${GIT_VERSION_PATCHLEVEL})
	set(${_appname}_VERSION ${GIT_VERSION_MAJOR}.${GIT_VERSION_MINOR}.${GIT_VERSION_PATCHLEVEL})

	if(Git_FOUND)
		message("  Got version ${${_appname}_VERSION}")
	endif()
endmacro()
