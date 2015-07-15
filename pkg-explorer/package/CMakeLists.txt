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
#      Copyright (c) 2006-2013 Made to Order Software Corp.
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

set( LOCAL_WPKG_ROOT "${CMAKE_BINARY_DIR}/WPKG" )

if( NOT EXISTS "${WPKG_ROOT}" )
    message( FATAL_ERROR "WPKG_ROOT=${WPKG_ROOT} not found!" )
endif()

if( MO_WINDOWS )
    unset( QT4_DLL_LIST_RELEASE )
    foreach( qt_lib ${QT4_REQ_LIBS} )
        list( APPEND QT4_DLL_LIST_RELEASE ${QT_BINARY_DIR}/${qt_lib}4.dll  )
    endforeach()

    install(
        FILES
            ${QT4_DLL_LIST_RELEASE}
        DESTINATION
            bin
        COMPONENT
            qt4
       )
endif()


if( ${CMAKE_BUILD_TYPE} STREQUAL "Debug" )
    message( STATUS "Not building debug packages" )
    return()
endif()

set( SUBPACKAGES "runtime*, documentation" )
set( COMPONENT_LIST runtime documentation )
set( INSTALL_PACKAGES wpkg-gui wpkggui-doc )
set( PLATFORM_DEPENDS "Depends/runtime: wpkg (>= 0.9.0), wpkggui-doc (>= ${VERSION})" )
if( MO_WINDOWS )
    set( PLATFORM_DEPENDS "${PLATFORM_DEPENDS}, wpkgguiqt4 (>= ${VERSION})" )
    set( SUBPACKAGES "${SUBPACKAGES}, qt4" )
    list( APPEND INSTALL_PACKAGES wpkgguiqt4 )
    list( APPEND COMPONENT_LIST qt4 )
endif()

configure_file( control.info.in "${LOCAL_WPKG_ROOT}/control.info" @ONLY )

unset( COMP_TARGETS )
foreach( _comp ${COMPONENT_LIST} )
    add_custom_target( ${_comp}_install
            COMMAND ${CMAKE_COMMAND}
                        -DCMAKE_INSTALL_PREFIX="${LOCAL_WPKG_ROOT}/${_comp}${INSTALL_PREFIX}"
                        -DCMAKE_INSTALL_COMPONENT=${_comp}
                        #-DCMAKE_INSTALL_DO_STRIP=1
                        -DBUILD_TYPE=$(Configuration)
                        -P ${CMAKE_BINARY_DIR}/cmake_install.cmake
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            DEPENDS package_prereqs
        )
    list( APPEND COMP_TARGETS ${_comp}_install )
    if( MSVC )
        set_property( TARGET ${_comp}_install PROPERTY FOLDER ${PACKAGE_FOLDER} )
        set_property( TARGET ${_comp}_install PROPERTY FOLDER ${PACKAGE_FOLDER} )
    endif()
endforeach()

find_program( WPKG wpkg${CMAKE_EXECUTABLE_SUFFIX} PATHS ${WPKG_ROOT}/bin )

add_custom_target( package
        COMMAND ${WPKG}
            --verbose
            --compressor gzip
            --output-dir "${WPKG_ROOT}/packages"
            --build      "${LOCAL_WPKG_ROOT}/control.info" "${LOCAL_WPKG_ROOT}"
        COMMAND ${WPKG}
            --verbose
            --create-index "${WPKG_ROOT}/packages/index.tar.gz"
            --repository "${WPKG_ROOT}/packages"
        COMMAND ${WPKG}
            --verbose
            --root    "${WPKG_ROOT}"
            --update
        COMMAND ${WPKG}
            --verbose
            --root    "${WPKG_ROOT}"
            --force-reinstall
            --install ${INSTALL_PACKAGES}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS
            ${COMP_TARGETS}
            "${WPKG_SOURCES_FILE}"
    )

set_property( TARGET package PROPERTY FOLDER ${PACKAGE_FOLDER} )

# vim: ts=4 sw=4 et
