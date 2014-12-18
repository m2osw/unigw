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

#===============================================================================
# Release version
#
set( VERSION_MAJOR 0 )
set( VERSION_MINOR 3 )
set( VERSION_PATCH 1 )
set( VERSION_DEVEL 0 )


#===============================================================================
# Set up system
#
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib" )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib" )


#===============================================================================
# Detect system paticulars
#
add_definitions( -DCMAKE_SYSTEM_NAME=\"${CMAKE_SYSTEM_NAME}\" )
if( UNIX )
	if( ${CMAKE_SYSTEM_NAME} MATCHES Linux )
		set( CANONALIZED_PREFIX linux )
		add_definitions( -DMO_LINUX )
		set( MO_LINUX TRUE )
	elseif( ${CMAKE_SYSTEM_NAME} MATCHES Darwin )
		set( CANONALIZED_PREFIX darwin )
		add_definitions( -DMO_DARWIN )
		set( MO_DARWIN TRUE )
	else()
		message( FATAL_ERROR "Unix-like platform '${CMAKE_SYSTEM_NAME}' not supported!" )
	endif()
	execute_process( COMMAND uname -m OUTPUT_VARIABLE ARCH OUTPUT_STRIP_TRAILING_WHITESPACE )
	if( (${ARCH} MATCHES i686) OR (${ARCH} MATCHES i386) )
		set( CANONALIZED_SUFFIX i386 )
	else( ${ARCH} MATCHES x86_64 )
		set( CANONALIZED_SUFFIX amd64 )
	else()
		message( FATAL_ERROR "Unknown architecture '${ARCH}'!" )
    endif()
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
		set( CANONALIZED_SUFFIX amd64 )
		set( WIN_MACHINE_FLAGS "/MACHINE:X64" )
	else()
		set( CANONALIZED_SUFFIX i386 )
		set( WIN_MACHINE_FLAGS "/MACHINE:X86" )
	endif()
	set( CANONALIZED_PREFIX win32 )
	add_definitions( -DMO_WINDOWS -DMO_MSVC -DNOMINMAX )
	set( MO_WINDOWS TRUE )
elseif( NOT BORLAND AND NOT CYGWIN )
	set( CANONALIZED_SUFFIX i386 )
	set( CANONALIZED_PREFIX mingw32 )
	add_definitions( -DMO_WINDOWS -DMO_MINGW32 )
	set( MO_WINDOWS TRUE )
	set( MO_MINGW32 TRUE )
else()
	message( FATAL_ERROR "Platform not supported!" )
endif()
set( CANONALIZED_ARCH "${CANONALIZED_PREFIX}-${CANONALIZED_SUFFIX}" )
add_definitions( -DCANONALIZED_ARCH=\"${CANONALIZED_ARCH}\" )

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

if( MO_WINDOWS )
	foreach( _lib ${QT4_REQ_LIBS} )
		set( Release_DLL "${QT_BINARY_DIR}/${_lib}4.dll" )
		set( Debug_DLL "${QT_BINARY_DIR}/${_lib}d4.dll" )
		#
		if( DEFINED CMAKE_BUILD_TYPE )
			#
			# Nmake build
			#
			file( INSTALL "${${CMAKE_BUILD_TYPE}_DLL}"
				DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
				)
		else()
			#
			# Visual Studio appends the Debug/Release folder to the output name
			#
			file( INSTALL "${Release_DLL}"
				DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release"
				)
			file( INSTALL "${Debug_DLL}"
				DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug"
				)
		endif()
		#
		install(
			FILES "${Release_DLL}"
			DESTINATION bin
			CONFIGURATIONS Release
			COMPONENT runtime
			)
		install(
			FILES "${Debug_DLL}"
			DESTINATION bin
			CONFIGURATIONS Debug
			COMPONENT runtime
			)
	endforeach()
endif()

set( DIST_PATH "$ENV{DIST_PATH}" CACHE PATH "Distribution path to find dependencies from WPKG." ) 

find_library( DEBPACKAGES_LIBRARY debpackages PATH "${DIST_PATH}/lib" )
find_library( EXPR_LIBRARY        expr        PATH "${DIST_PATH}/lib" )
find_library( UTF8_LIBRARY        utf8        PATH "${DIST_PATH}/lib" )
find_library( BZ2_LIBRARY         wpkg_bz2    PATH "${DIST_PATH}/lib" )
find_library( TLD_LIBRARY         wpkg_tld    PATH "${DIST_PATH}/lib" )
find_library( ZLIB_LIBRARY        wpkg_z      PATH "${DIST_PATH}/lib" )
find_program( WPKG                wpkg        PATH "${DIST_PATH}/bin" )
#
find_file( WPKGAR_H_INCLUDE wpkgar.h PATHS ${DIST_PATH}/include/wpkg/libdebpackages /usr/include/wpkg/libdebpackages /usr/local/include/wpkg/libdebpackages )
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
		COMPONENT runtime
		)
else()
	add_custom_target(changelog.Debian.gz ALL
		COMMAND ${WPKG} -z 9 -Z gzip --force-hold --force-overwrite --compress ${PROJECT_SOURCE_DIR}/Changelog --output-filename ${PROJECT_BINARY_DIR}/changelog.Debian.gz
		DEPENDS Changelog
	)

	install(
		FILES
			${CMAKE_SOURCE_DIR}/COPYING
			${CMAKE_SOURCE_DIR}/AUTHORS
			${CMAKE_SOURCE_DIR}/documentation/copyright
			${CMAKE_SOURCE_DIR}/documentation/index.html
			${PROJECT_BINARY_DIR}/changelog.Debian.gz
		DESTINATION share/doc/pkg-explorer
		COMPONENT runtime
		)
endif()

add_subdirectory( package )

set(CPACK_PACKAGE_NAME "pkg-explorer")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Multi-platform Packager Explorer GUI")
set(CPACK_PACKAGE_DESCRIPTION "Provides a Qt4-based GUI wrapper around the libdebpackages library provided by Made to Order Software.")
set(CPACK_PACKAGE_VENDOR "Made to Order Software Corporation")
set(CPACK_PACKAGE_CONTACT "contact@m2osw.com")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/COPYING")
set(CPACK_PACKAGE_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
set(CPACK_PACKAGE_VERSION_MAJOR "${VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${VERSION_PATCH}")

if( MO_LINUX )
	set(CPACK_GENERATOR "TGZ" "DEB" )
	## Debian/Ubuntu specific settings
	##
	set(CPACK_DEBIAN_PACKAGE_SECTION "admin")
	set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://sourceforge.net/projects/unigw/")
	set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Made to Order Software Corporation <contact@m2osw.com>")
	set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.15), libstdc++6 (>= 4.7.2), libgcc1 (>= 1:4.7.2), libqtcore4(>= 4.8.3), libqtgui4 (>= 4.8.3), libqtwebkit4 (>= 2.2.1)")
elseif( MO_DARWIN )
    set(CPACK_GENERATOR "TGZ" "DragNDrop")
	set(CPACK_PACKAGE_EXECUTABLES "pkg-explorer" "Package Explorer - GUI wrapper for WPKG (Windows Packager)")
	set(CPACK_PACKAGING_INSTALL_PREFIX "/")
elseif( MO_WINDOWS )
	set( CPACK_GENERATOR "ZIP" "NSIS" )
	set(CPACK_PACKAGE_INSTALL_DIRECTORY "M2OSW\\\\PackageExplorer")
	set(ICON_FILE "${CMAKE_SOURCE_DIR}/src/icons/m2osw_logo.png")
	file( TO_NATIVE_PATH "${ICON_FILE}" WIN_ICON_PATH )
	set(CPACK_PACKAGE_ICON "${WIN_ICON_PATH}")
	set(CPACK_NSIS_INSTALLED_ICON_NAME "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/bin/pkg-explorer.exe")
	set(CPACK_NSIS_DISPLAY_NAME "M2OSW\\Package Explorer for Windows")
	set(CPACK_NSIS_HELP_LINK "http://www.windowspackager.org/")
	set(CPACK_NSIS_URL_INFO_ABOUT "http://www.windowspackager.org/")
	set(CPACK_NSIS_CONTACT "contact@m2osw.com")
	set(CPACK_NSIS_MODIFY_PATH ON)
	set(CPACK_PACKAGE_EXECUTABLES "pkg-explorer" "Package Explorer - GUI wrapper for WPKG (Windows Packager)")
endif()
set(CPACK_SOURCE_GENERATOR "TGZ")
## At this point we don't include our contrib folder archives since
## they are available as is already (i.e. zlib and bz2, zip is unused)
set(CPACK_SOURCE_IGNORE_FILES "/CVS/;/\\\\.svn/;/work-files/;\\\\.swp;\\\\.deb;.*~;cscope.*;/tmp/;a.txt;/BUILD/;/Build/;/build/;zip_.*\\\\.tar\\\\.gz;bzip2-.*\\\\.tar\\\\.gz;zlib-.*\\\\.tar\\\\.bz2;xz-.*\\\\.tar\\\\.gz;changelog.html")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "pkg-explorer_${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
include(CPack)

## vim: ts=4 sw=4 noexpandtab
