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

#if( MSVC AND NOT (${CMAKE_MAKE_PROGRAM} STREQUAL "nmake") AND NOT (${CMAKE_MAKE_PROGRAM} STREQUAL "jom") )
#    message( STATUS "Not building packages due to devenv." )
#    return()
#endif()

if( MO_MINGW32 )
    get_filename_component( MINGW32_PATH ${CMAKE_MAKE_PROGRAM} DIRECTORY )
    file( GLOB MINGW32_DLL_LIST "${MINGW32_PATH}/*.dll" )
    install(
        FILES
            ${MINGW32_DLL_LIST}
        DESTINATION
            bin
        COMPONENT
            mingw32
       )
endif()

set( LOCAL_WPKG_ROOT "${CMAKE_BINARY_DIR}/WPKG" )
if( UNIX )
    set( INSTALL_PREFIX "/usr" )
else()
    set( INSTALL_PREFIX "" )
endif()

if( NOT EXISTS "${WPKG_ROOT}" )
    file( MAKE_DIRECTORY "${WPKG_ROOT}" )
endif()

if( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )
    message( WARNING "Debug packging not supported...yet..." )
    return()
else()
    set( SUBPACKAGES "runtime*, development, documentation, headers" )
    set( COMPONENT_LIST runtime development documentation headers )
    set( INSTALL_PACKAGES wpkg wpkg-dev wpkg-doc wpkg-hdrs-dev )
    set( PRIORITY required )
    if( MO_WINDOWS )
        set( PLATFORM_DEPENDS "Depends/runtime: msvc-runtime (= 12.0.0)" )
    elseif( MO_MINGW32 )
        set( SUBPACKAGES "${SUBPACKAGES}, mingw32" )
        set( PLATFORM_DEPENDS "Depends/runtime: wpkgmingw32 (>= \${wpkg:version})" )
        list( APPEND INSTALL_PACKAGES wpkgmingw32 )
        list( APPEND COMPONENT_LIST mingw32 )
    endif()
endif()

configure_file( admindb_init.txt "${LOCAL_WPKG_ROOT}/admindb_init.txt" @ONLY )
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

set( WPKG_CONTROL_FILE "${WPKG_ROOT}/var/lib/wpkg/core/control"      )
set( WPKG_SOURCES_FILE "${WPKG_ROOT}/var/lib/wpkg/core/sources.list" )

if( CMAKE_CROSSCOMPILING )
    set( WPKG "${wpkg_static_BINARY_DIR}/wpkg_static${CMAKE_EXECUTABLE_SUFFIX}" )
else()
    set( WPKG "wpkg_static" )
endif()

add_custom_command(
        OUTPUT "${WPKG_CONTROL_FILE}"
        COMMAND ${WPKG}
            --verbose
            --root "${WPKG_ROOT}"
            --create-admindir ${LOCAL_WPKG_ROOT}/admindb_init.txt
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

add_custom_command(
        OUTPUT "${WPKG_SOURCES_FILE}"
        COMMAND ${WPKG}
            --verbose
            --root "${WPKG_ROOT}"
            --add-sources "wpkg file://${WPKG_ROOT}/packages/ ."
        DEPENDS "${WPKG_CONTROL_FILE}"
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

if( MO_WINDOWS )
    set( MSVC_REDIST 
        COMMAND ${WPKG}
            --verbose
            --compressor gzip
            --output-dir "${WPKG_ROOT}/packages"
            --build ${CMAKE_CURRENT_SOURCE_DIR}/vcredist
    )
endif()
    
add_custom_target( build_packages
        ${MSVC_REDIST}
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
            --running-copy
            --force-reinstall
            --install ${INSTALL_PACKAGES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        DEPENDS
            ${COMP_TARGETS}
            "${WPKG_SOURCES_FILE}"
    )

set_property( TARGET build_packages PROPERTY FOLDER ${PACKAGE_FOLDER} )

# vim: ts=4 sw=4 et
