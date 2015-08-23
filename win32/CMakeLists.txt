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
#      Copyright (c) 2015 Made to Order Software Corp.
#
#      https://sourceforge.net/projects/unigw/
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

## This project builds the wpkg packages based on component.

if( MSVC AND NOT (${CMAKE_MAKE_PROGRAM} STREQUAL "nmake") AND NOT (${CMAKE_MAKE_PROGRAM} STREQUAL "jom") )
    message( STATUS "Not building packages due to devenv." )
    return()
endif()

find_program( NSIS makensis PATH "c:\\Program Files (x86)\\NSIS" )

if( NOT EXISTS "${NSIS}" )
    message( FATAL_ERROR "NSIS is needed! Please install." )
endif()

unigw_ParseVersion( WPKG ${UNIGW_VERSION} )

file( TO_NATIVE_PATH "${WPKG_ROOT}"                NATIVE_DIST_DIR   )
file( TO_NATIVE_PATH "${CMAKE_BINARY_DIR}"         NATIVE_BUILD_DIR  )
file( TO_NATIVE_PATH "${CMAKE_CURRENT_SOURCE_DIR}" NATIVE_SOURCE_DIR )
add_custom_target( nsis_package ALL
        COMMAND ${NSIS}
                    /V4
                    /DWPKG_VERSION_MAJOR="${WPKG_VERSION_MAJOR}"
                    /DWPKG_VERSION_MINOR="${WPKG_VERSION_MINOR}"
                    /DWPKG_VERSION_PATCH="${WPKG_VERSION_PATCH}"
                    /DDIST_DIR="${NATIVE_DIST_DIR}"
                    /DBUILD_DIR="${NATIVE_BUILD_DIR}"
                    /DSOURCE_DIR="${NATIVE_SOURCE_DIR}"
                    ${CMAKE_CURRENT_SOURCE_DIR}/setup.nsi
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS build-all
    )

# vim: ts=4 sw=4 et
