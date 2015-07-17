#
# File:
#      tools/CMakeLists.txt
#
# Description:
#      Tools to manipulate full featured Debian like packages.
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2006-2015 Made to Order Software Corp.
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

set( TOOLS_FOLDER tools )

#####################################################################
##
## dirsize
##

project(dirsize)

include_directories(
    "${PROJECT_BINARY_DIR}"
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/.."
    "${debpackages_BINARY_DIR}"
    "${debpackages_SOURCE_DIR}"
    "${wpkg_bz2_SOURCE_DIR}"
    "${wpkg_z_SOURCE_DIR}"
    "${expr_SOURCE_DIR}"
    "${utf8_SOURCE_DIR}"
    "${wpkg_tld_BINARY_DIR}"
)

add_executable( ${PROJECT_NAME}
    dirsize.cpp
    license.cpp
)

if( MSVC )
    set_target_properties( ${PROJECT_NAME} PROPERTIES FOLDER ${TOOLS_FOLDER} )
endif()

#StaticCompile()

target_link_libraries( ${PROJECT_NAME}
    debpackages
    expr
    wpkg_z
    wpkg_bz2
)

install(
    TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    COMPONENT runtime
)

install(
    FILES ${CMAKE_SOURCE_DIR}/documentation/${PROJECT_NAME}.1.gz
    DESTINATION share/man/man1
    COMPONENT documentation
)

#####################################################################
##
## debversion
##

project(debversion)

add_executable( ${PROJECT_NAME}
    debversion.cpp
    license.cpp
)


if( MSVC )
    set_target_properties( ${PROJECT_NAME} PROPERTIES FOLDER ${TOOLS_FOLDER} )
endif()

target_link_libraries( ${PROJECT_NAME}
    debpackages
)

install(
    TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    COMPONENT runtime
)

install(
    FILES ${CMAKE_SOURCE_DIR}/documentation/${PROJECT_NAME}.1.gz
    DESTINATION share/man/man1
    COMPONENT documentation
)


#####################################################################
##
## dar
##

project(dar)

add_executable( ${PROJECT_NAME}
    dar.cpp
    license.cpp
)

if( MSVC )
    set_target_properties( ${PROJECT_NAME} PROPERTIES FOLDER ${TOOLS_FOLDER} )
endif()

target_link_libraries( ${PROJECT_NAME}
    debpackages
)

install(
    TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    COMPONENT runtime
)

install(
    FILES ${CMAKE_SOURCE_DIR}/documentation/${PROJECT_NAME}.1.gz
    DESTINATION share/man/man1
    COMPONENT documentation
)


#####################################################################
##
## deb2graph
##

project(deb2graph)

add_executable( ${PROJECT_NAME}
    deb2graph.cpp
    license.cpp
)

if( MSVC )
    set_target_properties( ${PROJECT_NAME} PROPERTIES FOLDER ${TOOLS_FOLDER} )
endif()

target_link_libraries( ${PROJECT_NAME}
    debpackages
    expr
    wpkg_z
    wpkg_bz2
    wpkg_tld
)

install(
    TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    COMPONENT runtime
)

install(
    FILES ${CMAKE_SOURCE_DIR}/documentation/${PROJECT_NAME}.1.gz
    DESTINATION share/man/man1
    COMPONENT documentation
)


#####################################################################
##
## deb2html
##

project(deb2html)

add_executable( ${PROJECT_NAME}
    deb2html.cpp
    license.cpp
)

if( MSVC )
    set_target_properties( ${PROJECT_NAME} PROPERTIES FOLDER ${TOOLS_FOLDER} )
endif()

target_link_libraries( ${PROJECT_NAME}
    debpackages
    expr
    wpkg_z
    wpkg_bz2
    wpkg_tld
)

install(
    TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    COMPONENT runtime
)

install(
    FILES ${CMAKE_SOURCE_DIR}/documentation/${PROJECT_NAME}.1.gz
    DESTINATION share/man/man1
    COMPONENT documentation
)


#####################################################################
##
## rc-version
##

project(rc-version)

add_executable( ${PROJECT_NAME}
    rc-version.cpp
    license.cpp
)

if( MSVC )
    set_target_properties( ${PROJECT_NAME} PROPERTIES FOLDER ${TOOLS_FOLDER} )
endif()

target_link_libraries( ${PROJECT_NAME}
    debpackages
    expr
    wpkg_z
    wpkg_bz2
    wpkg_tld
)

install(
    TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    COMPONENT runtime
)

install(
    FILES ${CMAKE_SOURCE_DIR}/documentation/${PROJECT_NAME}.1.gz
    DESTINATION share/man/man1
    COMPONENT documentation
)


#####################################################################
##
## wpkg
##
project(wpkg)

add_executable( ${PROJECT_NAME}
    wpkg.cpp
    license.cpp
)

if( MSVC )
    set_target_properties( ${PROJECT_NAME} PROPERTIES FOLDER ${TOOLS_FOLDER} )
endif()

target_link_libraries( ${PROJECT_NAME}
    debpackages
    expr
    wpkg_z
    wpkg_bz2
    wpkg_tld
)

#if( MINGW32 OR MSYS )
#    # The -static-... flags do not seem to have an effect at this point
#    # (I also tried with just -static and the C & C++ libraries are still added!)
#    set_target_properties(wpkg PROPERTIES LINK_FLAGS "-static-libgcc -static-libstdc++" )
#endif( MINGW32 OR MSYS )

install(
    TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    COMPONENT runtime
)

project(wpkg_static)

add_executable( ${PROJECT_NAME}
    wpkg.cpp
    license.cpp
)

StaticCompile()

target_link_libraries( ${PROJECT_NAME}
    debpackages_static
    expr_static
    wpkg_z_static
    wpkg_bz2_static
    wpkg_tld_static
)

if( MO_MINGW32 OR MO_MINGW64 OR MSYS )
    # The -static-... flags do not seem to have an effect at this point
    # (I also tried with just -static and the C & C++ libraries are still added!)
    set_target_properties( ${PROJECT_NAME} PROPERTIES
        LINK_FLAGS "-static-libgcc -static-libstdc++"
        )
elseif( MSVC )
    set_target_properties( ${PROJECT_NAME} PROPERTIES
        FOLDER      ${TOOLS_FOLDER}
        OUTPUT_NAME wpkg_static
        )
endif()

install(
    TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    COMPONENT runtime
)

set_property( GLOBAL APPEND PROPERTY ALL_LIBRARY_TARGETS
    dirsize
    debversion
    dar
    deb2graph
    deb2html
    rc-version
    wpkg
    wpkg_static
)


#####################################################################
##
## Other installs
##
install(
    FILES ${CMAKE_SOURCE_DIR}/documentation/wpkg.1.gz
    DESTINATION share/man/man1
    COMPONENT documentation
)

file(
    GLOB MAN5 ${CMAKE_SOURCE_DIR}/documentation/*.5.gz
)
install(
    FILES ${MAN5}
    DESTINATION share/man/man5
    COMPONENT documentation
)

add_custom_target( tools_target DEPENDS dirsize debversion dar deb2graph deb2html rc-version wpkg wpkg_static )

if( MSVC )
    set_property( TARGET tools_target PROPERTY FOLDER ${TOOLS_FOLDER} )
endif()

# vim: ts=4 sw=4 et

