##===============================================================================
## Copyright (c) 2013 by Made to Order Software Corporation
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

project( pkg-explorer )

set( VERSION "${GUI_VERSION_MAJOR}.${GUI_VERSION_MINOR}.${GUI_VERSION_PATCH}" )
add_definitions( -DVERSION=\"${VERSION}\" )

#if( MO_WINDOWS )
#	list( APPEND QT4_REQ_LIBS phonon )
#else()
#	list( APPEND QT4_REQ_LIBS QtDBus )
#endif()

cmake_policy( SET CMP0020 NEW )

unset( PLATFORM_RESOURCES )
if( MO_WINDOWS )
	set( ICON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/icons/logo.ico" )
	set( VERSION_FULL "${GUI_VERSION_MAJOR},${GUI_VERSION_MINOR},${GUI_VERSION_PATCH},${GUI_VERSION_DEVEL}" )
	set( PLATFORM_RESOURCES ${PROJECT_BINARY_DIR}/resources.rc )
	configure_file( ${PROJECT_SOURCE_DIR}/resources.rc.in ${PLATFORM_RESOURCES} @ONLY )
	unset( QT5_LIBS )
elseif( MO_LINUX )
    set( QT5_LIBS "Qt5::DBus" )
    set( QT5_SUBLIBS DBus )
elseif( MO_DARWIN )
	set( PLATFORM_RESOURCES "icons/m2osw_logo.icns" )
	set_source_files_properties(
		"${PLATFORM_RESOURCES}"
		PROPERTIES
		MACOSX_PACKAGE_LOCATION Resources
		)
	list( APPEND QT5_LIBS "-framework IOKit" )
endif()
list( APPEND QT5_LIBS Qt5::Core Qt5::Widgets Qt5::WebKit Qt5::WebKitWidgets )

set( QT5_INSTALL_PATH "$ENV{QT5_INSTALL_PATH}" CACHE PATH "Root of qt5 installation." )
find_package( Qt5 REQUIRED Widgets WebKit WebKitWidgets ${QT5_SUBLIBS} PATHS ${QT5_INSTALL_PATH}/lib/cmake )

include_directories( ${PROJECT_SOURCE_DIR} ${PROJECT_BINARY_DIR} ${DEBPACKAGES_INCLUDE} )
link_directories( ${DIST_ROOT}/lib )

set( DEBUG_LIBRARIES
	debug debpackages_d
	debug expr_d
	debug utf8_d
	debug wpkg_bz2_d
	debug wpkg_tld_d
	debug wpkg_z_d
	)
set( RELEASE_LIBRARIES
	optimized debpackages
	optimized expr
	optimized utf8
	optimized wpkg_bz2
	optimized wpkg_tld
	optimized wpkg_z
	)

if( CMAKE_BUILD_TYPE )
	if( ${CMAKE_BUILD_TYPE} STREQUAL "Debug" )
		set( WPKG_LIBRARIES ${DEBUG_LIBRARIES} )
	else()
		set( WPKG_LIBRARIES ${RELEASE_LIBRARIES} )
	endif()
else()
	set( WPKG_LIBRARIES ${DEBUG_LIBRARIES} ${RELEASE_LIBRARIES} )
endif()

add_definitions( ${Qt5Widgets_DEFINITIONS} )


###############################################################################
##
## pkg-explorer

set( AUTOUIC_OPTIONS -n )

add_executable(
    ${PROJECT_NAME}
	WIN32
	MACOSX_BUNDLE
    ${PLATFORM_RESOURCES}
	database.h
	database.cpp
    include_qt4.h
	ImportDialog.ui
	ImportDialog.cpp
	InitThread.cpp
	InitThread.h
	InstallDialog.ui
	InstallDialog.cpp
	InstallThread.h
	InstallThread.cpp
    LicenseBox.ui
	LogForm.ui
	LogForm.cpp
	LogOutput.cpp
	Manager.h
	Manager.cpp
	MainWindow.ui
	MainWindow.cpp
	PrefsDialog.ui
	PrefsDialog.cpp
	ProcessDialog.ui
	ProcessDialog.cpp
	RemoveDialog.ui
	RemoveDialog.cpp
	RepoUtils.h
	RepoUtils.cpp
	SourceInfoDialog.ui
	SourceInfoDialog.cpp
	SourcesDialog.ui
	SourcesDialog.cpp
	UpdateThread.cpp
	UpdateThread.h
	WebForm.ui
	WebForm.cpp
	LicenseBox.ui
	LicenseBox.cpp
	main.cpp
    resources.qrc
	)

if( MSVC )
	set_target_properties( ${PROJECT_NAME} PROPERTIES
		LINK_FLAGS ${WIN_MACHINE_FLAGS}
	)
endif()

target_link_libraries( ${PROJECT_NAME}
	${QT5_LIBS}
	${EXTRA_LIBRARIES}
	${WPKG_LIBRARIES}
	)

set_target_properties( ${PROJECT_NAME} PROPERTIES
	OUTPUT_NAME_DEBUG   pkg-explorer-d
	OUTPUT_NAME_RELEASE pkg-explorer
	)

if( MO_DARWIN )
	set_target_properties( ${PROJECT_NAME}
			PROPERTIES
			MACOSX_BUNDLE_INFO_STRING "WPKG Package Explorer"
			MACOSX_BUNDLE_ICON_FILE "m2osw_logo.icns"
			MACOSX_BUNDLE_GUI_IDENTIFIER "Package Explorer"
			MACOSX_BUNDLE_LONG_VERSION_STRING "${GUI_VERSION_MAJOR}.${GUI_VERSION_MINOR}.${GUI_VERSION_PATCH}"
			MACOSX_BUNDLE_BUNDLE_NAME "Package Explorer"
			MACOSX_BUNDLE_SHORT_VERSION_STRING "${GUI_VERSION_MAJOR}.${GUI_VERSION_MINOR}"
			MACOSX_BUNDLE_COPYRIGHT "(c) 2013 by Made to Order Software Corporation"
			LINK_SEARCH_END_STATIC true
			OUTPUT_NAME "PackageExplorer"
			)

	install( TARGETS ${PROJECT_NAME}
			RUNTIME DESTINATION bin
			BUNDLE DESTINATION .
			COMPONENT runtime
			)
else()
	# Somehow the BUNDLE DESTINATION seems to break the RUNTIME DESTINATION
	# (maybe it overwrites it?)
	install( TARGETS ${PROJECT_NAME}
			RUNTIME DESTINATION bin
			COMPONENT runtime
			)
endif()

if( NOT MO_DARWIN )
	install( FILES ${CMAKE_CURRENT_SOURCE_DIR}/icons/logo.png
		DESTINATION share/icons/
		RENAME pkg-explorer.png
		COMPONENT documentation
		)
endif()

if( MO_LINUX )
	configure_file(
		${PROJECT_SOURCE_DIR}/pkg-explorer.desktop.in
		${PROJECT_BINARY_DIR}/pkg-explorer.desktop
		@ONLY
		)
	install( FILES ${PROJECT_BINARY_DIR}/pkg-explorer.desktop
		DESTINATION share/applications
		COMPONENT documentation
		)
endif()

## vim: ts=4 sw=4 noexpandtab
