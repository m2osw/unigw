#
# File:
#      CMakeLists.txt
#
# Description:
#      Definitions to create the build environment with cmake
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2011-2013 Made to Order Software Corp.
#
#      http://snapwebsites.org/
#      contact@m2osw.com
#
#      Permission is hereby granted, free of charge, to any person obtaining a
#      copy of this software and associated documentation files (the
#      "Software"), to deal in the Software without restriction, including
#      without limitation the rights to use, copy, modify, merge, publish,
#      distribute, sublicense, and/or sell copies of the Software, and to
#      permit persons to whom the Software is furnished to do so, subject to
#      the following conditions:
#
#      The above copyright notice and this permission notice shall be included
#      in all copies or substantial portions of the Software.
#
#      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
#      OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
#      MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
#      IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
#      CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
#      TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
#      SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#


cmake_minimum_required(VERSION 3.3.0)

project( unigw )

################################################################################
# Load up necessary modules
#
set( CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake )
find_package( WpkgBuild REQUIRED )

set( UNIGW_VERSION "1.0.1" CACHE STRING "WPKG version." )


################################################################################
# WPKG libraries
#
# NOTE: I'm disabling all of the debug packages because they really aren't needed
# (you can build your own debug version from source), and also because it's a right pain
# in the keester.
#
unigw_ConfigureMakeProject(
    PROJECT_NAME wpkg
    BUILD_TYPE   Release
    CONFIG_ARGS  -DWPKG_VERSION=${UNIGW_VERSION}
)

unigw_ConfigureMakeProject(
    PROJECT_NAME pkg-explorer
    DEPENDS      wpkg-Release
    BUILD_TYPE   Release
    CONFIG_ARGS  -DWPKG_VERSION=${UNIGW_VERSION}
)

get_property( BUILD_TARGETS GLOBAL PROPERTY BUILD_TARGETS )
get_property( CLEAN_TARGETS GLOBAL PROPERTY CLEAN_TARGETS )
#
if( UNIX )
    get_property( PACKAGE_TARGETS  GLOBAL PROPERTY PACKAGE_TARGETS  )
    get_property( DPUT_TARGETS     GLOBAL PROPERTY DPUT_TARGETS     )
    get_property( PBUILDER_TARGETS GLOBAL PROPERTY PBUILDER_TARGETS )
endif()

add_custom_target( build-all ALL
    DEPENDS ${BUILD_TARGETS}
    )
add_custom_target( clean-all
    COMMAND ${CMAKE_COMMAND} ${CMAKE_BINARY_DIR}
    DEPENDS ${CLEAN_TARGETS}
    )

if( MSVC OR MINGW )
    add_subdirectory( win32 )
endif()

if( UNIX )
    add_custom_target( debuild
        DEPENDS ${PACKAGE_TARGETS}
        )
    add_custom_target( dput
        DEPENDS ${DPUT_TARGETS}
        )
    add_custom_target( pbuilder
        DEPENDS ${PBUILDER_TARGETS}
        )
endif()

# vim: ts=4 sw=4 et nocindent
