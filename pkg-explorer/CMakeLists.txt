##===============================================================================
## Copyright (c) 2005-2013 by Made to Order Software Corporation
## 
## All Rights Reserved.
## 
## The source code in this file ("Source Code") is provided by Made to Order Software Corporation
## to you under the terms of the GNU General Public License, version 2.0
## ("GPL").  Terms of the GPL can be found in doc/GPL-license.txt in this distribution.
## 
## By copying, modifying or distributing this software, you acknowledge
## that you have read and understood your obligations described above,
## and agree to abide by those obligations.
## 
## ALL SOURCE CODE IN THIS DISTRIBUTION IS PROVIDED "AS IS." THE AUTHOR MAKES NO
## WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
## COMPLETENESS OR PERFORMANCE.
##===============================================================================
cmake_minimum_required (VERSION 2.8.7)

project( pkg_explorer_project )

# For DEVENV, organize projects into folders
#
set_property( GLOBAL PROPERTY USE_FOLDERS ON )

option( BUILD_WPKG "Build the wpkg package" ON )

#===============================================================================
# Release version
#
set( VERSION_MAJOR 0 )
set( VERSION_MINOR 3 )
set( VERSION_PATCH 3 )
set( VERSION_DEVEL 0 )

set( VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}-${VERSION_DEVEL}" )

#===============================================================================
# Set up system
#
if( CMAKE_BUILD_TYPE )
   set( CMAKE_PDB_OUTPUT_DIRECTORY     ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE} )
   set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE} )
   set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE} )
   set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE} )
else()
   set( CMAKE_PDB_OUTPUT_DIRECTORY     ${CMAKE_BINARY_DIR} )
   set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR} )
   set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR} )
   set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR} )
endif()


#===============================================================================
# Detect system paticulars
#
# TODO: This code needs to be placed in a stand-alone cmake library/module.
#
add_definitions( -DCMAKE_SYSTEM_NAME=\"${CMAKE_SYSTEM_NAME}\" )
if( UNIX )
	if( ${CMAKE_SYSTEM_NAME} MATCHES Linux )
		add_definitions( -DMO_LINUX )
		set( MO_LINUX TRUE )
	elseif( ${CMAKE_SYSTEM_NAME} MATCHES Darwin )
		add_definitions( -DMO_DARWIN )
		set( MO_DARWIN TRUE )
	else()
		message( FATAL_ERROR "Unix-like platform '${CMAKE_SYSTEM_NAME}' not supported!" )
	endif()
	execute_process( COMMAND uname -m OUTPUT_VARIABLE ARCH OUTPUT_STRIP_TRAILING_WHITESPACE )
    #
    # Feel free to comment out this line if you don't like the warnings
    # HOWEVER: do not submit code to us if it generates warnings!
    #
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wall -Wextra -pedantic -std=c++0x -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-include-dirs -Wnoexcept -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=1 -Wundef -Wno-unused -Wno-variadic-macros -Wno-parentheses -Wno-unknown-pragmas -fdiagnostics-show-option" )
	set( CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG}  -O0" )
	set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELASE} -O3" )
elseif( MSVC )
	set( CMAKE_CXX_FLAGS
		"${CMAKE_CXX_FLAGS} -D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS /EHsc /Zc:wchar_t"
	)
	if( CMAKE_CL_64 )
		set( WIN_MACHINE_FLAGS "/MACHINE:X64" )
	else()
		set( WIN_MACHINE_FLAGS "/MACHINE:X86" )
	endif()
	add_definitions( -DMO_WINDOWS -DMO_MSVC -DNOMINMAX )
	set( MO_WINDOWS TRUE )
elseif( MINGW )
    set( WIN_MACHINE_FLAGS "-fPIC" )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++0x" )
    add_definitions( -DMO_WINDOWS -DMO_MINGW32 )
	set( MO_WINDOWS TRUE )
	set( MO_MINGW32 TRUE )
else()
	message( FATAL_ERROR "Platform not supported!" )
endif()

#===============================================================================
# Find dependencies
#
set( QT4_REQ_LIBS QtCore QtGui QtNetwork QtWebKit )
if( MO_WINDOWS )
	list( APPEND QT4_REQ_LIBS phonon )
else()
	list( APPEND QT4_REQ_LIBS QtDBus )
endif()

find_package( Qt4 4.7.4 REQUIRED ${QT4_REQ_LIBS} )
set( QT_USE_PHONON FALSE )
set( QT_USE_QTMAIN TRUE )
set( QT_USE_QTCORE TRUE )
include( ${QT_USE_FILE} )
add_definitions( ${QT_DEFINITIONS} )
set( CMAKE_AUTOMOC TRUE )

set( ENV_ROOT_PATH "$ENV{WPKG_ROOT}" )
if( NOT ENV_ROOT_PATH )
    set( ENV_ROOT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../dist" )
endif()
set( WPKG_ROOT "${ENV_ROOT_PATH}" CACHE PATH "Root of wpkg installation." )

if( UNIX )
    set( INSTALL_PREFIX "/usr" )
else()
    set( INSTALL_PREFIX "" )
endif()
set( DIST_ROOT "${WPKG_ROOT}${INSTALL_PREFIX}" )

find_program( WPKG wpkg_static PATH "${DIST_ROOT}/bin" )

find_file( WPKGAR_H_INCLUDE wpkgar.h PATHS ${DIST_ROOT}/include/wpkg/libdebpackages /usr/include/wpkg/libdebpackages /usr/local/include/wpkg/libdebpackages )
if( NOT WPKGAR_H_INCLUDE )
	message( FATAL_ERROR "You must specify the location of the wpkgar.h file!" )
endif()

get_filename_component( DEBPACKAGES_DIR  ${WPKGAR_H_INCLUDE}  PATH )
get_filename_component( DEBPACKAGES_IDIR ${DEBPACKAGES_DIR} PATH )
set( DEBPACKAGES_INCLUDE "${DEBPACKAGES_DIR}" "${DEBPACKAGES_IDIR}" )

if( MO_WINDOWS )
	set( EXTRA_LIBRARIES wsock32 ws2_32 mpr )
endif()


#===============================================================================
# Include source
#
add_subdirectory(src)


#===============================================================================
#
# Packaging
#
if( MO_DARWIN )
	install(
		FILES
			${CMAKE_SOURCE_DIR}/COPYING
			${CMAKE_SOURCE_DIR}/AUTHORS
			${CMAKE_SOURCE_DIR}/documentation/copyright
			${CMAKE_SOURCE_DIR}/documentation/index.html
		DESTINATION .
		COMPONENT documentation
		)
else()
	set( CHANGELOG_OUTPUT ${PROJECT_BINARY_DIR}/changelog.Debian.gz )
	add_custom_command( OUTPUT ${CHANGELOG_OUTPUT}
		COMMAND ${WPKG} -z 9 -Z gzip --force-hold --force-overwrite --compress ${PROJECT_SOURCE_DIR}/Changelog --output-filename ${PROJECT_BINARY_DIR}/changelog.Debian.gz
		DEPENDS Changelog
	)
	add_custom_target( changelog_generation ALL DEPENDS ${CHANGELOG_OUTPUT} )
	set_property( TARGET changelog_generation PROPERTY FOLDER package )

	install(
		FILES
			${CMAKE_SOURCE_DIR}/COPYING
			${CMAKE_SOURCE_DIR}/AUTHORS
			${CMAKE_SOURCE_DIR}/documentation/copyright
			${CMAKE_SOURCE_DIR}/documentation/index.html
			${CHANGELOG_OUTPUT}
		DESTINATION share/doc/pkg-explorer
		COMPONENT documentation
		)
endif()

add_custom_target( package_prereqs DEPENDS changelog_generation pkg-explorer pkg-installer )
set_property( TARGET package_prereqs PROPERTY FOLDER package )
add_subdirectory( package )

## vim: ts=4 sw=4 noexpandtab
