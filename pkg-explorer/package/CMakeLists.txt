#
# File:
#      CMakeLists.txt
#
# Description:
#      Definitions to create wpkg packages from the binary
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2013 Made to Order Software Corp.
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

## This project builds the pkg-explorer package based on component.

set( DEBVERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}-${VERSION_DEVEL}" )
set( WPKG_ROOT "${CMAKE_BINARY_DIR}/WPKG" )
set( PACKAGE_FOLDER package )
if( UNIX )
    set( INSTALL_PREFIX "/usr" )
else()
    set( INSTALL_PREFIX "" )
endif()
set( COMPONENT "runtime" )
configure_file( control.${COMPONENT}.in "${WPKG_ROOT}/${COMPONENT}/WPKG/control" @ONLY )
add_custom_target( ${COMPONENT}_install
        COMMAND ${CMAKE_COMMAND}
                    -DCMAKE_INSTALL_PREFIX="${WPKG_ROOT}/${COMPONENT}${INSTALL_PREFIX}"
                    -DCMAKE_INSTALL_COMPONENT=${COMPONENT}
                    -DCMAKE_INSTALL_DO_STRIP=1
                    -DBUILD_TYPE=$(Configuration)
                    -P ${CMAKE_BINARY_DIR}/cmake_install.cmake
        #COMMAND ${CMAKE_COMMAND}
        #            -DCMAKE_INSTALL_PREFIX="${WPKG_ROOT}/${COMPONENT}${INSTALL_PREFIX}"
        #            -DCMAKE_INSTALL_COMPONENT=package
        #            -DBUILD_TYPE=$(Configuration)
        #            -P ${CMAKE_BINARY_DIR}/cmake_install.cmake
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS package_prereqs
    )
add_custom_target( ${COMPONENT}
        COMMAND ${WPKG}
            --output-dir "${WPKG_ROOT}/packages"
            --build      "${WPKG_ROOT}/${COMPONENT}"
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS ${COMPONENT}_install
        )
set_property( TARGET ${COMPONENT}_install PROPERTY FOLDER ${PACKAGE_FOLDER} )
set_property( TARGET ${COMPONENT} PROPERTY FOLDER ${PACKAGE_FOLDER} )

add_custom_target( package DEPENDS runtime )
set_property( TARGET package PROPERTY FOLDER ${PACKAGE_FOLDER} )

# vim: ts=4 sw=4 et
