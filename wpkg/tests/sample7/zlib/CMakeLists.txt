#
# CMakeLists.txt for zlib as a project on its own built with the
# wpkg tool.
#

##
## Version of project
##
set(ZLIB_VERSION_MAJOR 1)
set(ZLIB_VERSION_MINOR 2)
set(ZLIB_VERSION_PATCH 8)

cmake_minimum_required(VERSION 2.8)

##
## Project
##
project(z)

set(VERSION ${ZLIB_VERSION_MAJOR}.${ZLIB_VERSION_MINOR}.${ZLIB_VERSION_PATCH})
set(CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
set(INSTALL_LIB_DIR ${CMAKE_INSTALL_PREFIX}/lib)
set(INSTALL_INC_DIR ${CMAKE_INSTALL_PREFIX}/include)

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/zlib.pc.cmakein ${CMAKE_CURRENT_BINARY_DIR}/zlib.pc @ONLY )

if ( MSVC )
    set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE" )
endif()

set(ZLIB_PUBLIC_HDRS
    zconf.h
    zlib.h
)

add_library( ${PROJECT_NAME} SHARED
    adler32.c
    compress.c
    crc32.c
    deflate.c
    gzclose.c
    gzlib.c
    gzread.c
    gzwrite.c
    infback.c
    inffast.c
    inflate.c
    inftrees.c
    trees.c
    uncompr.c
    zutil.c
)



if( MSVC )
    set_target_properties( ${PROJECT_NAME} PROPERTIES
        COMPILE_FLAGS -DZLIB_DLL
    )
else()
    set_target_properties( ${PROJECT_NAME} PROPERTIES
        VERSION ${ZLIB_VERSION_MAJOR}.${ZLIB_VERSION_MINOR}
        SOVERSION ${ZLIB_VERSION_MAJOR}
    )
endif()

install(
    TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin COMPONENT runtime
    LIBRARY DESTINATION lib COMPONENT runtime
    ARCHIVE DESTINATION lib COMPONENT development
)
install(
    FILES ${ZLIB_PUBLIC_HDRS}
    DESTINATION include
    COMPONENT development
)
install(
    FILES ${CMAKE_BINARY_DIR}/zlib.pc
    DESTINATION share/pkgconfig
    COMPONENT development
)
install(
    FILES zlib.3
    DESTINATION share/man
    COMPONENT documentation
)
install(
    FILES zlib.3.pdf README FAQ
    DESTINATION share/doc/zlib
    COMPONENT documentation
)

# TBD: are there unit tests we could run? (see test folder)
add_custom_target( run_unit_tests
    COMMAND echo "Done. No unit tests ran, though."
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS ${PROJECT_NAME}
)

##
## Packaging
##
set(CPACK_PACKAGE_NAME "zlib")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Multi-platform packager")
set(CPACK_PACKAGE_VENDOR "Made to Order Software Corporation")
set(CPACK_PACKAGE_CONTACT "contact@m2osw.com")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/README")
set(CPACK_GENERATOR "TGZ" "DEB")
set(CPACK_SOURCE_GENERATOR "TGZ")
## At this point we don't include our contrib folder archives since
## they are available as is already (i.e. zlib and bz2, zip is unused)
set(CPACK_SOURCE_IGNORE_FILES "/CVS/;/\\\\.svn/;/work-files/;\\\\.swp;\\\\.deb;.*~;cscope.*;/tmp/;a.txt;/BUILD/;/Build/;/build/;libtld-.*\\\\.tar\\\\.gz;zip_.*\\\\.tar\\\\.gz;bzip2-.*\\\\.tar\\\\.gz;zlib-.*\\\\.tar\\\\.bz2;xz-.*\\\\.tar\\\\.gz;changelog.html")
set(CPACK_PACKAGE_VERSION "${ZLIB_VERSION_MAJOR}.${ZLIB_VERSION_MINOR}.${ZLIB_VERSION_PATCH}")
set(CPACK_PACKAGE_VERSION_MAJOR "${ZLIB_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${ZLIB_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${ZLIB_VERSION_PATCH}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "zlib_${ZLIB_VERSION_MAJOR}.${ZLIB_VERSION_MINOR}.${ZLIB_VERSION_PATCH}")
set(CPACK_COMPONENTS_ALL development runtime documentation)
set(CPACK_DEB_COMPONENT_INSTALL ON)

## Debian/Ubuntu specific settings
##
set(CPACK_DEBIAN_PACKAGE_SECTION "admin")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://sourceforge.net/projects/unigw/")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Made to Order Software Corporation <contact@m2osw.com>")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.13), libstdc++6 (>= 4.6.1-9ubuntu3), libgcc1 (>= 1:4.6.1)")
include(CPack)

# vim: ts=4 sw=4 et
