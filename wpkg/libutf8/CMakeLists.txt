#
# File:
#      libutf8/CMakeLists.txt
#
# Description:
#      A simple to use C++ library to convert strings to wstrings and
#      vice versa assuming that strings are UTF-8 and wstrings are
#      either UCS-2 or UCS-4.
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2013-2015 Made to Order Software Corp.
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
## libutf8_static
##

# WARNING: C/CXX flags are defined once for all the CMakeLists.txt projects
if( MSVC )
    set( UTF8_FOLDER libs/utf8 )
    # disable warnings about dll linkage concerns but we do not have any inlines (warning 4251)
    set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4251" )
endif ( MSVC )

project(utf8_static)

include_directories(
    "${PROJECT_SOURCE_DIR}"
)

file( GLOB LIBUTF8_INCLUDE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/libutf8/*.h" )
set( LIBUTF8_SOURCE_FILES libutf8.cpp )


add_library( ${PROJECT_NAME} STATIC
    ${LIBUTF8_INCLUDE_FILES}
    ${LIBUTF8_SOURCE_FILES}
)

StaticCompile()

if( MSVC )
    set_target_properties( ${PROJECT_NAME} PROPERTIES
        LINK_FLAGS ${WIN_MACHINE_FLAGS}
        FOLDER ${UTF8_FOLDER}
    )
else()
    set_target_properties( ${PROJECT_NAME} PROPERTIES
        VERSION ${WPKG_VERSION_MAJOR}.${WPKG_VERSION_MINOR}
        SOVERSION ${WPKG_VERSION_MAJOR}
    )
endif()

install(
    TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin COMPONENT runtime
    LIBRARY DESTINATION lib COMPONENT development
    ARCHIVE DESTINATION lib COMPONENT development
)



##
## libutf8
##

project(utf8)

include_directories(
    "${PROJECT_SOURCE_DIR}"
)

add_library( ${PROJECT_NAME} SHARED
    ${LIBUTF8_INCLUDE_FILES}
    ${LIBUTF8_SOURCE_FILES}
)

if( MSVC )
    set_target_properties( ${PROJECT_NAME} PROPERTIES
		COMPILE_FLAGS "-DLIBUTF8_DLL -Dlibutf8_EXPORTS"
        LINK_FLAGS ${WIN_MACHINE_FLAGS}
        FOLDER ${UTF8_FOLDER}
    )
else()
    set_target_properties( ${PROJECT_NAME} PROPERTIES
        VERSION ${WPKG_VERSION_MAJOR}.${WPKG_VERSION_MINOR}
        SOVERSION ${WPKG_VERSION_MAJOR}
    )
endif()

install(
    TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin COMPONENT runtime
    LIBRARY DESTINATION lib COMPONENT runtime
    ARCHIVE DESTINATION lib COMPONENT development
)
install(
    DIRECTORY ${PROJECT_SOURCE_DIR}/libutf8
    DESTINATION include/wpkg
    COMPONENT headers
    FILES_MATCHING PATTERN *.h
    PATTERN .svn EXCLUDE
)

set_property( GLOBAL APPEND PROPERTY ALL_LIBRARY_TARGETS utf8 utf8_static )

add_custom_target( utf8_target DEPENDS utf8_static utf8 )
set_property( TARGET utf8_target PROPERTY FOLDER ${UTF8_FOLDER} )


# vim: ts=4 sw=4 et
