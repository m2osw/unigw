#
# File:
#      CMakeLists.txt
#
# Description:
#      Definitions to create all the unit tests of the wpkg libraries and
#      tools.
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

# Note that if user does not have cppunit then this does not get included
# in the final output
project(unittest)

if( NOT CMAKE_MODULE_PATH )
    set( CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake )
endif()
include( FindCppUnit )

if( CPPUNIT_FOUND )

	add_definitions( -DNOQT )

    #set( HAVE_QT4 1 )
	#add_definitions( -DHAVE_QT4 )
    #find_package( Qt4 4.8.1 REQUIRED ${ARGN} )
    #include( ${QT_USE_FILE} )
    #add_definitions( ${QT_DEFINITIONS} )

    include_directories(
		"${CMAKE_BINARY_DIR}"
        "${PROJECT_BINARY_DIR}"
        "${PROJECT_BINARY_DIR}/../../libdebpackages"
        "${PROJECT_BINARY_DIR}/../../libtld"
        "${PROJECT_SOURCE_DIR}"
        "${PROJECT_SOURCE_DIR}/../.."
        "${PROJECT_SOURCE_DIR}/../../bzip2"
        "${PROJECT_SOURCE_DIR}/../../libdebpackages"
        "${PROJECT_SOURCE_DIR}/../../libutf8"
        "${PROJECT_SOURCE_DIR}/../../libexpr"
        "${PROJECT_BINARY_DIR}/../../libexpr"
        "${PROJECT_SOURCE_DIR}/../../libtld/include"
        "${PROJECT_SOURCE_DIR}/../../zlib"
        "${CPPUNIT_INCLUDE_DIR}"
    )

    add_executable( ${PROJECT_NAME}
        unittest_main.cpp
        unittest_advgetopt.cpp
        unittest_architecture.cpp
        unittest_control.cpp
        unittest_expr.cpp
        unittest_libutf8.cpp
        unittest_memfile.cpp
        unittest_package.cpp
        unittest_system.cpp
        unittest_uri_filename.cpp
        unittest_version.cpp
        ../../tools/license.cpp
    )

    if( HAVE_QT4 )
        set( CPPUNIT_QT4 qxcppunitd qxrunnerd )
    endif( HAVE_QT4 )

    target_link_libraries( ${PROJECT_NAME}
        debpackages_static
        expr_static
        wpkg_z_static
        wpkg_bz2_static
        ${CPPUNIT_LIBRARIES}
        ${CPPUNIT_QT4}
    )

    add_custom_target( run_unit_tests
        COMMAND mkdir -p tmp && tests/unittests/unittest
            --tmp tmp --wpkg tools/wpkg
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS ${PROJECT_NAME}
    )

endif( CPPUNIT_FOUND )
# vim: ts=4 sw=4 et
