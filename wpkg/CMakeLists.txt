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
cmake_minimum_required(VERSION 3.3.0)

##
## Initialization
##
set(WPKG_VERSION_MAJOR 0)
set(WPKG_VERSION_MINOR 9)
set(WPKG_VERSION_PATCH 11)

enable_testing()

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
elseif( MINGW )
    set( WPKG_PROCESSOR i386 )
    set( WPKG_OS mswindows )
    # WIN_MACHINE_FLAGS cannot be empty and since cmake adds -fPIC under
    # our feet, that makes no difference at this point...
    set( WIN_MACHINE_FLAGS "-fPIC" )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++0x" )
    add_definitions( -DMO_WINDOWS -DMO_MINGW32 -DCONNECT_COMMANDLINE=2048 )
    set( MO_WINDOWS TRUE )
    set( MO_MINGW32 TRUE )
else()
    message( FATAL_ERROR "Platform not supported!" )
endif()
set( WPKG_ARCHITECTURE "${WPKG_OS}-${WPKG_PROCESSOR}" )

set( ENV_ROOT_PATH "$ENV{WPKG_ROOT}" )
if( NOT ENV_ROOT_PATH )
    set( ENV_ROOT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../dist" )
endif()
set( WPKG_ROOT "${ENV_ROOT_PATH}" CACHE PATH "Root of wpkg installation." )

if( DEFINED ENV{CONTROLLED_VARS_DEBUG} )
    add_definitions( -DCONTROLLED_VARS_DEBUG )
endif()

set( ADVGETOPT_THROW_FOR_EXIT $ENV{ADVGETOPT_THROW_FOR_EXIT} CACHE BOOL "Throw on exit for testing." )
if( ADVGETOPT_THROW_FOR_EXIT )
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

## Tools and others
add_subdirectory( tools )
add_subdirectory( tests/unittests )
add_subdirectory( documentation )

## Make sure we have "_d" for debug binaries
get_property( LIBRARY_TARGETS GLOBAL PROPERTY ALL_LIBRARY_TARGETS )
foreach( TGT ${LIBRARY_TARGETS} )
    set_property( TARGET ${TGT} PROPERTY OUTPUT_NAME_DEBUG ${TGT}_d )
endforeach()

install(
    DIRECTORY "${CMAKE_SOURCE_DIR}/documentation/."
    DESTINATION share/doc/wpkg
    COMPONENT documentation
    PATTERN .svn EXCLUDE
    PATTERN copyright EXCLUDE
)

if( MSVC )
    set( PACKAGE_FOLDER package )
endif()

add_custom_target( package_prereqs DEPENDS bz2_target zlib_target tld_target utf8_target expr_target debpackages_target tools_target )

if( MSVC )
    set_property( TARGET package_prereqs PROPERTY FOLDER ${PACKAGE_FOLDER} )
endif()

add_subdirectory( package )

# vim: ts=4 sw=4 et
