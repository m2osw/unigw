#
# File:         CMakeLists.txt
# Object:       Definitions to create the build environment with cmake
#
# Copyright:    Copyright (c) 2011-2013 Made to Order Software Corp.
#               All Rights Reserved.
#
# http://snapwebsites.org/
# contact@m2osw.com
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

set(LIBTLD_VERSION_MAJOR 1)
set(LIBTLD_VERSION_MINOR 4)
set(LIBTLD_VERSION_PATCH 0)

cmake_minimum_required(VERSION 2.8)
project(wpkg_tld)

# WARNING: C/CXX flags are defined once for all the CMakeLists.txt projects
if ( MSVC )
    set( TLD_FOLDER libs/tld )
    # disable warnings about dll linkage concerns but we do not have any inlines (warning 4251)
    set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4251" )
    # disable warnings about dll linkage concerns but we do not have any inlines (warning 4251)
    set ( CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} /wd4251 /EHsc" )
endif ( MSVC )

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/libtld/tld.h.in ${CMAKE_CURRENT_BINARY_DIR}/libtld/tld.h)
include_directories(
    "${PROJECT_BINARY_DIR}"
    "${PROJECT_SOURCE_DIR}"
)
add_library(${PROJECT_NAME} SHARED
    src/tld.c
    src/tld_data.c
    src/tld_object.cpp
)
if( MSVC )
    set_target_properties( ${PROJECT_NAME} PROPERTIES
            COMPILE_FLAGS "-DLIBTLD_DLL -Dwpkg_tld_EXPORTS"
            LINK_FLAGS ${WIN_MACHINE_FLAGS}
            FOLDER ${TLD_FOLDER}
        )
else()
    set_target_properties( ${PROJECT_NAME} PROPERTIES
        VERSION ${WPKG_VERSION_MAJOR}.${WPKG_VERSION_MINOR}
        SOVERSION ${WPKG_VERSION_MAJOR}
    )
endif()
install(
    TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin COMPONENT runtime
    LIBRARY DESTINATION lib COMPONENT runtime
    ARCHIVE DESTINATION lib COMPONENT development
)

##
## TLD static library
##
project(wpkg_tld_static)
include_directories(
    "${PROJECT_BINARY_DIR}"
    "${PROJECT_SOURCE_DIR}"
)
add_library(${PROJECT_NAME} STATIC
    src/tld.c
    src/tld_data.c
    src/tld_object.cpp
)
StaticCompile()
if( MSVC )
    set_target_properties( ${PROJECT_NAME} PROPERTIES
        LINK_FLAGS ${WIN_MACHINE_FLAGS}
        FOLDER ${TLD_FOLDER}
    )
elseif( NOT MO_CYGWIN AND NOT MO_MINGW32 AND NOT MO_MINGW64 )
    # We need the -fPIC to use this library as extension of PHP, etc.
    set_target_properties(${PROJECT_NAME} PROPERTIES
            COMPILE_FLAGS -fPIC
        )
    set_target_properties( ${PROJECT_NAME} PROPERTIES
        VERSION ${WPKG_VERSION_MAJOR}.${WPKG_VERSION_MINOR}
        SOVERSION ${WPKG_VERSION_MAJOR}
    )
endif()


install(
    TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin COMPONENT runtime
    LIBRARY DESTINATION lib COMPONENT development
    ARCHIVE DESTINATION lib COMPONENT development
)
install(
    DIRECTORY ${PROJECT_SOURCE_DIR}/include/libtld
    DESTINATION include/wpkg
    COMPONENT development
    FILES_MATCHING PATTERN *.h
    PATTERN .svn EXCLUDE
)
install(
    FILES ${PROJECT_BINARY_DIR}/libtld/tld.h
    DESTINATION include/wpkg/libtld
    COMPONENT development
)

set_property( GLOBAL APPEND PROPERTY ALL_LIBRARY_TARGETS wpkg_tld wpkg_tld_static )

add_custom_target( tld_target DEPENDS wpkg_tld_static wpkg_tld )
set_property( TARGET tld_target PROPERTY FOLDER ${TLD_FOLDER} )

# vim: ts=4 sw=4 et
