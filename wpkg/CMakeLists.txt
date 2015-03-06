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
## Initialization
##
set(WPKG_VERSION_MAJOR 0)
set(WPKG_VERSION_MINOR 9)
set(WPKG_VERSION_PATCH 8)

cmake_minimum_required(VERSION 2.8.4)

project( wpkg_project )

# Look in our local module dir
set( CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake )

# If you redesign wpkg and your version becomes
# incompatible with ours, change this name!
set( WPKG_VENDOR m2osw )

# For DEVENV, organize projects into folders
#
set_property( GLOBAL PROPERTY USE_FOLDERS ON )

#===============================================================================
# Detect system particulars
#
add_definitions( -DCMAKE_SYSTEM_NAME=\"${CMAKE_SYSTEM_NAME}\" -D_WIN32_WINNT=0x0600 -DWINVER=0x0600 -D_WIN32_IE=0x0800 )
if( UNIX )
    if( ${CMAKE_SYSTEM_NAME} MATCHES Linux )
        set( WPKG_OS linux )
        add_definitions( -DMO_LINUX )
        set( MO_LINUX TRUE )
    elseif( ${CMAKE_SYSTEM_NAME} MATCHES Darwin )
        set( WPKG_OS darwin )
        add_definitions( -DMO_DARWIN )
        set( MO_DARWIN TRUE )
    elseif( ${CMAKE_SYSTEM_NAME} MATCHES CYGWIN )
        set( WPKG_OS cygwin )
        add_definitions( -DMO_CYGWIN )
        set( MO_CYGWIN TRUE )
    elseif( ${CMAKE_SYSTEM_NAME} MATCHES SunOS )
        set( WPKG_OS solaris )
        add_definitions( -DMO_SUNOS )
        set( MO_SUNOS TRUE )
    elseif( ${CMAKE_SYSTEM_NAME} MATCHES FreeBSD )
        set( WPKG_OS freebsd )
        add_definitions( -DMO_FREEBSD )
        set( MO_FREEBSD TRUE )
        include( dev/freebsd-toolchain.cmake )
    else()
        message( FATAL_ERROR "Unix-like platform '${CMAKE_SYSTEM_NAME}' not supported!" )
    endif()
    execute_process( COMMAND uname -m OUTPUT_VARIABLE ARCH OUTPUT_STRIP_TRAILING_WHITESPACE )
    if( (${ARCH} MATCHES i386) OR (${ARCH} MATCHES i686) )
        set( WPKG_PROCESSOR i386 )
    else( ${ARCH} MATCHES x86_64 )
        set( WPKG_PROCESSOR amd64 )
    else()
        message( FATAL_ERROR "Unknown architecture '${ARCH}'!" )
    endif()
    if( DEFINED ENV{WPKG_COVERAGE} )
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -Werror -Wall -Wextra -pedantic -std=gnu++0x -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Winit-self -Wlogical-op -Wmissing-include-dirs -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=1 -Wundef -Wno-unused -Wunused-variable -Wno-variadic-macros -Wno-parentheses -Wno-unknown-pragmas -Wwrite-strings -Wswitch -fdiagnostics-show-option -fprofile-arcs -ftest-coverage" )
        if( NOT MO_CYGWIN AND NOT MO_SUNOS )
            set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wnoexcept -Wold-style-cast -Wformat=2" )
        endif()
        set( CMAKE_C_FLAGS "-g -O0 -Wall -Wextra -fprofile-arcs -ftest-coverage" )
        set( CMAKE_SHARED_LINKER_FLAGS "-fprofile-arcs -ftest-coverage" )
        set( CMAKE_EXE_LINKER_FLAGS "-fprofile-arcs -ftest-coverage" )
        set( COVERAGE_LIBS gcov )
    else()
        # Feel free to comment out this line if you don't like the warnings
        # HOWEVER: do not submit code to us if it generates warnings!
        # You may want to keep the -O3 though
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wall -Wextra -pedantic -std=gnu++0x -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Winit-self -Wlogical-op -Wmissing-include-dirs -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=1 -Wundef -Wno-unused -Wunused-variable -Wno-variadic-macros -Wno-parentheses -Wno-unknown-pragmas -Wwrite-strings -Wswitch -fdiagnostics-show-option" )
        if( NOT MO_CYGWIN AND NOT MO_SUNOS )
            set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wnoexcept -Wold-style-cast -Wformat=2" )
        endif()
        set( OPT "-O3" )
        if( MO_DARWIN )
            message( WARNING "Disabling optimization for DARWIN, since the compiler generates code that crashes pkg_explorer! TODO: this needs to be addressed!" )
            set( OPT "-O0" )
        endif()
        set( CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG}   -O0"    )
        set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${OPT}" )
    endif()
elseif( MSVC )
    set( WPKG_OS mswindows )
    if( CMAKE_CL_64 )
        set( WPKG_PROCESSOR amd64 )
        set( WIN_MACHINE_FLAGS "/MACHINE:X64" )
    else()
        set( WPKG_PROCESSOR i386 )
        set( WIN_MACHINE_FLAGS "/MACHINE:X86" )
    endif()
    add_definitions( -DMO_WINDOWS )
    set( MO_WINDOWS TRUE )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNOMINMAX -D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS" )
    set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS" )
elseif( MINGW32 OR MSYS )
    set( WPKG_PROCESSOR i386 )
    set( WPKG_OS mingw32 )
    # WIN_MACHINE_FLAGS cannot be empty and since cmake adds -fPIC under
    # our feet, that makes no difference at this point...
    set( WIN_MACHINE_FLAGS "-fPIC" )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++0x" )
    add_definitions( -DMO_WINDOWS -DMO_MINGW32 -DCONNECT_COMMANDLINE=2048 )
    set( MO_WINDOWS TRUE )
    set( MO_MINGW32 TRUE )
elseif( MINGW )
    set( WPKG_PROCESSOR amd64 )
    set( WPKG_OS mingw64 )
    # WIN_MACHINE_FLAGS cannot be empty and since cmake adds -fPIC under
    # our feet, that makes no difference at this point...
    set( WIN_MACHINE_FLAGS "-fPIC" )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++0x" )
    add_definitions( -DMO_WINDOWS -DMO_MINGW64 -DCONNECT_COMMANDLINE=2048 )
    set( MO_WINDOWS TRUE )
    set( MO_MINGW64 TRUE )
else()
    message( FATAL_ERROR "Platform not supported!" )
endif()
set( WPKG_ARCHITECTURE "${WPKG_OS}-${WPKG_PROCESSOR}" )

if( DEFINED ENV{CONTROLLED_VARS_DEBUG} )
    add_definitions( -DCONTROLLED_VARS_DEBUG )
endif()
if( DEFINED ENV{ADVGETOPT_THROW_FOR_EXIT} )
    add_definitions( -DADVGETOPT_THROW_FOR_EXIT )
endif()


configure_file(
    ${PROJECT_SOURCE_DIR}/dev/control-bin.in
    ${PROJECT_BINARY_DIR}/dev/control-bin
)


function(StaticCompile)
    if(MSVC AND (${CMAKE_VERSION} VERSION_GREATER "2.8.11.2") )
        #
        # Static compile uses /MT and /MTd
        # In all other cases do nothing
        #
        # Thu Oct  3 12:11:44 PDT 2013 (RDB)
        # Not sure we want to do this anyway. Otherwise, we will need a static library that links against
        # the static version of the runtime c lib, and another that links against the dynamic version.
        #
        # For now, older versions of cmake won't croak here.
        #
        target_compile_options( ${PROJECT_NAME}
            PUBLIC "/MT$<$<STREQUAL:$<CONFIGURATION>,Debug>:d>"
        )
    endif()
endfunction()


## Libraries
add_subdirectory( bzip2              )
add_subdirectory( zlib               )
add_subdirectory( controlled_vars    )
add_subdirectory( libtld             )
add_subdirectory( libutf8            )
add_subdirectory( libexpr            )
add_subdirectory( libdebpackages     )

## Make sure we have "_d" for debug binaries
get_property( LIBRARY_TARGETS GLOBAL PROPERTY ALL_LIBRARY_TARGETS )
foreach( TGT ${LIBRARY_TARGETS} )
    set_property( TARGET ${TGT} PROPERTY OUTPUT_NAME_DEBUG ${TGT}_d )
endforeach()

## Tools and others
add_subdirectory( tools )
if( NOT MO_MINGW32 AND NOT MO_MINGW64 )
    add_subdirectory( tests/unittests )
endif()
add_subdirectory( documentation )

install(
    DIRECTORY "${CMAKE_SOURCE_DIR}/documentation/."
    DESTINATION share/doc/wpkg
    COMPONENT documentation
    PATTERN .svn EXCLUDE
    PATTERN copyright EXCLUDE
)

# Create a target which generates the changelog gzip file.
# Note: you specify "wpkg" as the tool since it is also a cmake target. add_custom_command() understands CMake generated targets.
#
#if( MO_MINGW32 )
## running wpkg doesn't work when cross compiling; use file as is for now
#set( CHANGELOG_OUTPUT_FILE "${PROJECT_SOURCE_DIR}/ChangeLog" )
#else( MO_MINGW32 )
#set( CHANGELOG_INPUT_FILE  "${PROJECT_SOURCE_DIR}/ChangeLog"           )
#set( CHANGELOG_OUTPUT_FILE "${PROJECT_BINARY_DIR}/changelog.Debian.gz" )
#add_custom_command(
#    OUTPUT "${CHANGELOG_OUTPUT_FILE}"
#    COMMAND wpkg -z 9 -Z gzip --force-hold --force-overwrite --compress "${CHANGELOG_INPUT_FILE}" --output-filename "${CHANGELOG_OUTPUT_FILE}"
#    DEPENDS "${CHANGELOG_INPUT_FILE}"
#)
#add_custom_target( changelog ALL DEPENDS "${CHANGELOG_OUTPUT_FILE}" )
#endif( MO_MINGW32 )

#install(
#    FILES
#        #"${CMAKE_SOURCE_DIR}/documentation/copyright"
#        "${CMAKE_SOURCE_DIR}/AUTHORS"
#        #"${CHANGELOG_OUTPUT_FILE}"
#    DESTINATION share/doc/wpkg
#    COMPONENT runtime
#)

if( MSVC )
    set( PACKAGE_FOLDER package )
endif()

add_custom_target( package_prereqs DEPENDS bz2_target zlib_target tld_target utf8_target expr_target debpackages_target tools_target )

if( MSVC )
    set_property( TARGET package_prereqs PROPERTY FOLDER ${PACKAGE_FOLDER} )
endif()

add_subdirectory( package )


##
## Packaging
##
set(CPACK_PACKAGE_NAME "wpkg")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Multi-platform packager")
set(CPACK_PACKAGE_VENDOR "Made to Order Software Corporation")
set(CPACK_PACKAGE_CONTACT "contact@m2osw.com")
set(CPACK_RESOURCE_FILE_LICENSE "${wpkg_project_SOURCE_DIR}/COPYING")
set(CPACK_GENERATOR "TGZ" "DEB")
set(CPACK_SOURCE_GENERATOR "TGZ")
## At this point we don't include our contrib folder archives since
## they are available as is already (i.e. zlib and bz2, zip is unused)
set(CPACK_SOURCE_IGNORE_FILES "/CVS/;/\\\\.svn/;/work-files/;\\\\.swp;\\\\.deb;.*~;cscope.*;/tmp/;a.txt;/BUILD/;/Build/;/build/;libtld-.*\\\\.tar\\\\.gz;zip_.*\\\\.tar\\\\.gz;bzip2-.*\\\\.tar\\\\.gz;zlib-.*\\\\.tar\\\\.bz2;xz-.*\\\\.tar\\\\.gz;changelog.html")
set(CPACK_PACKAGE_VERSION "${WPKG_VERSION_MAJOR}.${WPKG_VERSION_MINOR}.${WPKG_VERSION_PATCH}")
set(CPACK_PACKAGE_VERSION_MAJOR "${WPKG_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${WPKG_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${WPKG_VERSION_PATCH}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "wpkg_${WPKG_VERSION_MAJOR}.${WPKG_VERSION_MINOR}.${WPKG_VERSION_PATCH}")
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
