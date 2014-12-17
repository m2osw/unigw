cmake_minimum_required (VERSION 2.8.7)

project( systray )

find_package( Qt4 4.7.4 REQUIRED QtCore QtGui )
set( QT_USE_PHONON FALSE )
set( QT_USE_QTMAIN TRUE )
include( ${QT_USE_FILE} )
add_definitions( ${QT_DEFINITIONS} )
set( CMAKE_AUTOMOC TRUE )

include_directories( ${PROJECT_BINARY_DIR} )

QT4_WRAP_UI( UIC_FILES MainWindow.ui )

QT4_ADD_RESOURCES( RES_FILES resources.qrc )

add_executable( ${PROJECT_NAME} WIN32 main.cpp MainWindow.cpp ${UIC_FILES} ${RES_FILES} )

target_link_libraries( ${PROJECT_NAME} ${QT_LIBRARIES} )
