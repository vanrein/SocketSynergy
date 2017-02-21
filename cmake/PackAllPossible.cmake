# PackAllPossible.cmake -- find what target package formats can be made locally
#
# This package creates package formats for a maximum number of target formats:
#  - .tar.bz2 for source code
#  - .deb for Debian / Ubuntu / Mint Linux
#  - .rpm for RedHat / Fedora / SuSe Linux
#  - .pkg for Mac OS X
#
# The main output from this script is setting up the CPACK_GENERATOR list.
#
# The code below detects the available options automatically, by looking at
# their CPack generator support and the availability of the main driving
# program.  There are no warnings for missing out on options, so you simply
# may have to add such drivers if you intend to package for a wider range
# of target platforms.
#
# No notion is taken of machine formats, also because CMake might be used for
# cross-compiling.  Please do not forget to think for yourself.  Sure you can
# build a Windows package filled with Linux executables, but it is not going
# to be as useful as building a FreeBSD package with Linux executables.
#
#
# Copyright 2017, Rick van Rein <rick@openfortress.nl>
#
# Redistribution and use is allowed according to the terms of the two-clause BSD license.
#    https://opensource.org/licenses/BSD-2-Clause
#    SPDX short identifier: BSD-2-Clause


# Always produce a source tar ball
set (CPACK_GENERATOR TGZ;TBZ2)

# Support DEB packaging for Debian / Ubuntu / Mint Linux
find_program (PROGRAM_LINUX_DEBBUILD dpkg-buildpackage)
if (NOT ${PROGRAM_LINUX_DEBBUILD} STREQUAL "PROGRAM_LINUX_DEBBUILD-NOTFOUND")
	list (APPEND CPACK_GENERATOR DEB)
endif ()
unset (PROGRAM_LINUX_DEBBUILD CACHE)

# Support RPM packaging for RedHat / Fedora / SuSe Linux
find_program (PROGRAM_LINUX_RPMBUILD rpmbuild)
if (NOT ${PROGRAM_LINUX_RPMBUILD} STREQUAL "PROGRAM_LINUX_RPMBUILD-NOTFOUND")
	list (APPEND CPACK_GENERATOR RPM)
endif ()
unset (PROGRAM_LINUX_RPMBUILD CACHE)

# Support PackageMaker packaging for Mac OS X
find_program (PROGRAM_MACOSX_PKGBUILD pkgbuild)
if (NOT ${PROGRAM_MACOSX_PKGBUILD} STREQUAL "PROGRAM_MACOSX_PKGBUILD-NOTFOUND")
	list (APPEND CPACK_GENERATOR PackageMaker)
endif ()
unset (PROGRAM_MACOSX_PKGBUILD CACHE)

# Support NSIS packaging for Windows
find_program (PROGRAM_WINDOWS_NSISBUILD makensis)
if (NOT ${PROGRAM_WINDOWS_NSISBUILD} STREQUAL "PROGRAM_WINDOWS_NSISBUILD-NOTFOUND")
	list (APPEND CPACK_GENERATOR NSIS)
endif ()
unset (PROGRAM_WINDOWS_NSISBUILD CACHE)

# Publish the results in the global cached scope
set (CPACK_GENERATOR "${CPACK_GENERATOR}" CACHE STRING "CPack generators that will be run")
