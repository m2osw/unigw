#
# File:
#      CMakeLists.txt
#
# Description:
#      Root of tests folder
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2016 Made to Order Software Corp.
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

# Note that if user does not have Catch then this does not get included
# in the final output
project(tests)

if( NOT CMAKE_MODULE_PATH )
    set( CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake )
endif()

# The version of Catch that these tests are keyed to is from git: 1e2fld1
# Find it at https://github.com/philsquared/Catch.git
# For Ubuntu Linux, just use the catch package version 1.0+m10git1e2f1d16-1
#
# If you use the tip, you will get errors complaining that the "Clara" class
# doesn't exist.
#
# NOTE: I added the catch folder under tests, so that way you don't have to
# muss about trying to find the right version.
#
set( CATCH_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/catch" )
set( TEST_FOLDER "tests" )
add_definitions( -DCATCH_CONFIG_PREFIX_ALL )

# Include for every folder below
include_directories(
    "${wpkg_project_SOURCE_DIR}"
    "${wpkg_project_BINARY_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/common"
    "${debpackages_BINARY_DIR}"
    "${debpackages_SOURCE_DIR}"
    "${expr_BINARY_DIR}"
    "${expr_SOURCE_DIR}"
    "${utf8_BINARY_DIR}"
    "${utf8_SOURCE_DIR}"
    "${wpkg_bz2_BINARY_DIR}"
    "${wpkg_bz2_SOURCE_DIR}"
    "${wpkg_tld_BINARY_DIR}"
    "${wpkg_tld_SOURCE_DIR}/include"
    "${wpkg_z_BINARY_DIR}"
    "${wpkg_z_SOURCE_DIR}"
    "${CATCH_INCLUDE_DIR}"
)

set( TMPDIR "${CMAKE_BINARY_DIR}/tmp" )
if( NOT EXISTS ${TMPDIR} )
    file( MAKE_DIRECTORY ${TMPDIR} )
endif()

add_subdirectory( common           )
add_subdirectory( unittests        )
add_subdirectory( integrationtests )

# vim: ts=4 sw=4 et
