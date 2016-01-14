#
# File:
#      CMakeLists.txt
#
# Description:
#      Definitions to create all the unit tests of the wpkg libraries.
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2013-2015 Made to Order Software Corp.
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
project(unittest)

include_directories(
    "${PROJECT_BINARY_DIR}"
    "${PROJECT_SOURCE_DIR}"
)

add_executable( ${PROJECT_NAME}
    unittest_main.cpp
    unittest_advgetopt.cpp
    unittest_architecture.cpp
    unittest_control.cpp
    unittest_expr.cpp
    unittest_installer.cpp
    unittest_libutf8.cpp
    unittest_memfile.cpp
    unittest_uri_filename.cpp
    unittest_version.cpp
    ${CMAKE_SOURCE_DIR}/tools/license.cpp
)

target_link_libraries( ${PROJECT_NAME}
    debpackages_static
    expr_static
    wpkg_z_static
    wpkg_bz2_static
)

unset( DEBUG_WORD )
if( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )
    set( DEBUG_WORD "_d" )
endif()
add_test( unittests ${PROJECT_NAME} --tmp ${TMPDIR} )

# vim: ts=4 sw=4 et
