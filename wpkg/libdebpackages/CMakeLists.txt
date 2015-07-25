#
# File:
#      libdebpackages/CMakeLists.txt
#
# Description:
#      A library used to compare Debian compatible versions
#      between each others.
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


##
## libdebpackages
##

# WARNING: C/CXX flags are defined once for all the CMakeLists.txt projects
if( MSVC )
    set( DEBPACKAGES_FOLDER libs/debpackages )
    # disable warnings about dll linkage concerns but we do not have any inlines (warning 4251)
    set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4251" )
endif()
if( MSVC OR MO_MINGW32 OR MO_MINGW64 OR MSYS )
    set ( EXTRA_LIBRARIES wsock32 ws2_32 mpr ole32 uuid )
endif()

file( GLOB LIBDEBPACKAGES_INCLUDE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/libdebpackages/*.h" )

project( debpackages_static )

configure_file(
    ${PROJECT_SOURCE_DIR}/libdebpackages/debian_packages.h.in
    ${PROJECT_BINARY_DIR}/libdebpackages/debian_packages.h
)
list( APPEND LIBDEBPACKAGES_INCLUDE_FILES "${PROJECT_BINARY_DIR}/libdebpackages/debian_packages.h" )

include_directories(
    "${CMAKE_BINARY_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}/libdebpackages"
    "${wpkg_tld_BINARY_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
    "${wpkg_bz2_static_SOURCE_DIR}"
    "${wpkg_z_static_SOURCE_DIR}"
    "${expr_static_SOURCE_DIR}"
    "${expr_static_BINARY_DIR}"
    "${utf8_static_SOURCE_DIR}"
)

set(LIBDEBPACKAGES_SOURCE_FILES
    advgetopt.cpp
    case_insensitive_string.cpp
    compatibility.cpp
    debian_version.cpp
    libversion.c
    md5.cpp
    memfile.cpp
    strptime.c
    tcp_client_server.cpp
    wpkgar.cpp
    wpkgar_block.cpp
    wpkgar_build.cpp
    wpkgar_install.cpp
    wpkgar_remove.cpp
    wpkgar_repository.cpp
    wpkgar_tracker.cpp
    wpkg_architecture.cpp
    wpkg_backup.cpp
    wpkg_changelog.cpp
    wpkg_control.cpp
    wpkg_control_fields.cpp
    wpkg_control_file_item.cpp
    wpkg_copyright.cpp
    wpkg_dependencies.cpp
    wpkg_field.cpp
    wpkg_filename.cpp
    wpkg_output.cpp
    wpkg_stream.cpp
    wpkg_util.cpp
)

add_library( ${PROJECT_NAME} STATIC
    ${LIBDEBPACKAGES_INCLUDE_FILES}
    ${LIBDEBPACKAGES_SOURCE_FILES}
)

StaticCompile()

if( MSVC )
    set_target_properties( ${PROJECT_NAME} PROPERTIES
        LINK_FLAGS "${WIN_MACHINE_FLAGS}"
        FOLDER ${DEBPACKAGES_FOLDER}
    )
endif()

if( MO_SUNOS )
    target_link_libraries( ${PROJECT_NAME}
        nsl
        socket
        resolv
    )
endif()

target_link_libraries( ${PROJECT_NAME}
    expr_static
    wpkg_z_static
    wpkg_bz2_static
    wpkg_tld_static
    ${EXTRA_LIBRARIES}
)

set_target_properties( ${PROJECT_NAME} PROPERTIES
    VERSION ${WPKG_VERSION_MAJOR}.${WPKG_VERSION_MINOR}
    SOVERSION ${WPKG_VERSION_MAJOR}
)

install(
    TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin COMPONENT runtime
    LIBRARY DESTINATION lib COMPONENT development
    ARCHIVE DESTINATION lib COMPONENT development
)


##
## dynamic library version of libdebpackages
##

project( debpackages )

configure_file(
    ${PROJECT_SOURCE_DIR}/libdebpackages/debian_packages.h.in
    ${PROJECT_BINARY_DIR}/libdebpackages/debian_packages.h
)

include_directories(
    "${CMAKE_CURRENT_BINARY_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}/libdebpackages"
    "${wpkg_tld_BINARY_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
    "${wpkg_bz2_SOURCE_DIR}"
    "${wpkg_z_SOURCE_DIR}"
    "${expr_SOURCE_DIR}"
    "${utf8_SOURCE_DIR}"
)

add_library( ${PROJECT_NAME} SHARED
    ${LIBDEBPACKAGES_INCLUDE_FILES}
    ${LIBDEBPACKAGES_SOURCE_FILES}
)

if( MSVC )
    set_target_properties( ${PROJECT_NAME} PROPERTIES
        COMPILE_FLAGS -DDEBIAN_PACKAGE_DLL
        LINK_FLAGS ${WIN_MACHINE_FLAGS}
        FOLDER ${DEBPACKAGES_FOLDER}
    )
endif()

target_link_libraries( ${PROJECT_NAME}
    expr
    wpkg_z
    wpkg_bz2
    wpkg_tld
    ${EXTRA_LIBRARIES}
)

set_target_properties( ${PROJECT_NAME} PROPERTIES
    VERSION ${WPKG_VERSION_MAJOR}.${WPKG_VERSION_MINOR}
    SOVERSION ${WPKG_VERSION_MAJOR}
)

install(
    TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin COMPONENT runtime
    LIBRARY DESTINATION lib COMPONENT runtime
    ARCHIVE DESTINATION lib COMPONENT development
)
install(
    DIRECTORY ${PROJECT_SOURCE_DIR}/libdebpackages
    DESTINATION include/wpkg
    COMPONENT headers
    FILES_MATCHING PATTERN *.h
    PATTERN .svn EXCLUDE
)
install(
    FILES ${PROJECT_BINARY_DIR}/libdebpackages/debian_packages.h
    DESTINATION include/wpkg/libdebpackages
    COMPONENT headers
)

set_property( GLOBAL APPEND PROPERTY ALL_LIBRARY_TARGETS debpackages debpackages_static )

add_custom_target( debpackages_target DEPENDS debpackages_static debpackages )
if( MSVC )
    set_property( TARGET debpackages_target PROPERTY FOLDER ${DEBPACKAGES_FOLDER} )
endif()


# vim: ts=4 sw=4 et
