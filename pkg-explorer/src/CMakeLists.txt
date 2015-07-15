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

set( VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}" )
add_definitions( -DVERSION=\"${VERSION}\" )

unset( PLATFORM_RESOURCES )
if( MO_WINDOWS )
	set( ICON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/icons/logo.ico" )
	set( VERSION_FULL "${VERSION_MAJOR},${VERSION_MINOR},${VERSION_PATCH},${VERSION_DEVEL}" )
	set( PLATFORM_RESOURCES ${PROJECT_BINARY_DIR}/resources.rc )
	configure_file( ${PROJECT_SOURCE_DIR}/resources.rc.in ${PLATFORM_RESOURCES} @ONLY )
elseif( MO_DARWIN )
	set( PLATFORM_RESOURCES "icons/m2osw_logo.icns" )
	set_source_files_properties(
		"${PLATFORM_RESOURCES}"
		PROPERTIES
		MACOSX_PACKAGE_LOCATION Resources
		)
	list( APPEND QT_LIBRARIES "-framework IOKit" )
endif()

include_directories( ${PROJECT_SOURCE_DIR} ${PROJECT_BINARY_DIR} ${DEBPACKAGES_INCLUDE} )
link_directories( ${DIST_ROOT}/lib )

set( DEBUG_LIBRARIES
	debug ${LIBPREFIX}debpackages_d
	debug ${LIBPREFIX}expr_d
	debug ${LIBPREFIX}utf8_d
	debug ${LIBPREFIX}wpkg_bz2_d
	debug ${LIBPREFIX}wpkg_tld_d
	debug ${LIBPREFIX}wpkg_z_d
	)
set( RELEASE_LIBRARIES
	optimized ${LIBPREFIX}debpackages
	optimized ${LIBPREFIX}expr       
	optimized ${LIBPREFIX}utf8       
	optimized ${LIBPREFIX}wpkg_bz2   
	optimized ${LIBPREFIX}wpkg_tld   
	optimized ${LIBPREFIX}wpkg_z     
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


###############################################################################
##
## pkg-explorer

QT4_WRAP_UI( UIC_FILES
	ImportDialog.ui
	InstallDialog.ui
    LicenseBox.ui
	LogForm.ui
    MainWindow.ui
	PrefsDialog.ui
	ProcessDialog.ui
	RemoveDialog.ui
	SourceInfoDialog.ui
	SourcesDialog.ui
    WebForm.ui
    OPTIONS -n
    )

QT4_ADD_RESOURCES( RES_FILES
    resources.qrc
    )

add_executable(
    ${PROJECT_NAME}
	WIN32
	MACOSX_BUNDLE
	${UIC_FILES}
    ${RES_FILES}
    ${PLATFORM_RESOURCES}
	database.h
	database.cpp
    include_qt4.h
	ImportDialog.ui
	ImportDialog.cpp
	InstallDialog.ui
	InstallDialog.cpp
	InstallThread.h
	InstallThread.cpp
	LogForm.ui
	LogForm.cpp
	LogOutput.cpp
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
	WebForm.ui
	WebForm.cpp
	LicenseBox.ui
	LicenseBox.cpp
	main.cpp
	)

if( MSVC )
	set_target_properties( ${PROJECT_NAME} PROPERTIES
		LINK_FLAGS ${WIN_MACHINE_FLAGS}
	)
endif()

target_link_libraries( ${PROJECT_NAME}
	${QT_LIBRARIES}
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
			MACOSX_BUNDLE_LONG_VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}"
			MACOSX_BUNDLE_BUNDLE_NAME "Package Explorer"
			MACOSX_BUNDLE_SHORT_VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}"
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


###############################################################################
##
## pkg-installer

project( pkg-installer )

QT4_WRAP_UI( UIC_FILES
	ImportDialog.ui
	OPTIONS -n
	)

QT4_ADD_RESOURCES( RES_FILES
	resources.qrc
	)

add_executable(
	${PROJECT_NAME}
	WIN32
	${UIC_FILES}
	${RES_FILES}
	${PLATFORM_RESOURCES}
	database.h
	database.cpp
	include_qt4.h
	ImportDialog.ui
	ImportDialog.cpp
	InstallThread.h
	InstallThread.cpp
	LogForm.ui
	LogForm.cpp
	LogOutput.cpp
	ProcessDialog.h
	ProcessDialog.cpp
	ProcessWindow.h
	pkg-installer-main.cpp
	)

if( MSVC )
	set_target_properties( ${PROJECT_NAME} PROPERTIES
		LINK_FLAGS ${WIN_MACHINE_FLAGS}
	)
endif()

target_link_libraries( ${PROJECT_NAME}
	${QT_LIBRARIES}
	${EXTRA_LIBRARIES}
	${WPKG_LIBRARIES}
	)

set_target_properties( ${PROJECT_NAME} PROPERTIES
	OUTPUT_NAME_DEBUG   pkg-installer-d
	OUTPUT_NAME_RELEASE pkg-installer
	)

install( TARGETS ${PROJECT_NAME}
		RUNTIME DESTINATION bin
        COMPONENT runtime
		)


## vim: ts=4 sw=4 noexpandtab
