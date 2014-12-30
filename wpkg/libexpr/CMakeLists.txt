#
# File:
#      libexpr/CMakeLists.txt
#
# Description:
#      A C like expression evaluator in a simple to use library.
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2006-2014 Made to Order Software Corp.
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
## libexpr_static
##

# WARNING: C/CXX flags are defined once for all the CMakeLists.txt projects
if( MSVC )
    set( EXPR_FOLDER libs/expr )
    # disable warnings about dll linkage concerns but we do not have any inlines (warning 4251)
    set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4251" )
endif ( MSVC )

project(expr_static)

configure_file(
    ${PROJECT_SOURCE_DIR}/libexpr/expr.h.in
    ${PROJECT_BINARY_DIR}/libexpr/expr.h
)

include_directories(
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_BINARY_DIR}"
    "${utf8_SOURCE_DIR}"
)

file( GLOB LIBEXPR_INCLUDE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/libexpr/*.h" )
set( LIBEXPR_SOURCE_FILES expr.cpp variable.cpp )


add_library( ${PROJECT_NAME} STATIC
    ${LIBEXPR_INCLUDE_FILES}
    ${LIBEXPR_SOURCE_FILES}
)

StaticCompile()

if( MSVC )
    set_target_properties( ${PROJECT_NAME} PROPERTIES
        LINK_FLAGS ${WIN_MACHINE_FLAGS}
        FOLDER ${EXPR_FOLDER}
    )
else()
    set_target_properties( ${PROJECT_NAME} PROPERTIES
        VERSION ${WPKG_VERSION_MAJOR}.${WPKG_VERSION_MINOR}
        SOVERSION ${WPKG_VERSION_MAJOR}
    )
endif()

target_link_libraries( ${PROJECT_NAME}
    utf8_static
)

install(
    TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin COMPONENT runtime
    LIBRARY DESTINATION lib COMPONENT development
    ARCHIVE DESTINATION lib COMPONENT development
)



##
## libexpr
##

project(expr)

configure_file(
    ${PROJECT_SOURCE_DIR}/libexpr/expr.h.in
    ${PROJECT_BINARY_DIR}/libexpr/expr.h
)

add_library( ${PROJECT_NAME} SHARED
    ${LIBEXPR_INCLUDE_FILES}
    ${LIBEXPR_SOURCE_FILES}
)

if( MSVC )
    set_target_properties( ${PROJECT_NAME} PROPERTIES
            COMPILE_FLAGS "-DEXPR_DLL -Dexpr_EXPORTS"
            LINK_FLAGS ${WIN_MACHINE_FLAGS}
            FOLDER ${EXPR_FOLDER}
        )
else()
    set_target_properties( ${PROJECT_NAME} PROPERTIES
        VERSION ${WPKG_VERSION_MAJOR}.${WPKG_VERSION_MINOR}
        SOVERSION ${WPKG_VERSION_MAJOR}
    )
endif()

target_link_libraries( ${PROJECT_NAME}
    utf8
)

install(
    TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin COMPONENT runtime
    LIBRARY DESTINATION lib COMPONENT runtime
    ARCHIVE DESTINATION lib COMPONENT development
)

install(
    DIRECTORY ${PROJECT_SOURCE_DIR}/libexpr
    DESTINATION include/wpkg
    COMPONENT development
    FILES_MATCHING PATTERN *.h
    PATTERN .svn EXCLUDE
)


##
## c_expr tool
##

project(c_expr)

add_executable( ${PROJECT_NAME}
    c_expr.cpp
)

if( MSVC )
    set_target_properties( ${PROJECT_NAME} PROPERTIES
        LINK_FLAGS ${WIN_MACHINE_FLAGS}
        FOLDER ${EXPR_FOLDER}
    )
else()
    set_target_properties( ${PROJECT_NAME} PROPERTIES
        VERSION ${WPKG_VERSION_MAJOR}.${WPKG_VERSION_MINOR}
        SOVERSION ${WPKG_VERSION_MAJOR}
    )
endif()

target_link_libraries( ${PROJECT_NAME}
    expr
)

install(
    TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    COMPONENT runtime
)

install(
    FILES ${CMAKE_SOURCE_DIR}/documentation/${PROJECT_NAME}.1.gz
    DESTINATION share/man/man1
    COMPONENT documentation
)

add_custom_target( expr_target DEPENDS expr_static expr c_expr )

if( MSVC )
    set_property( TARGET expr_target PROPERTY FOLDER ${EXPR_FOLDER} )
endif()


# vim: ts=4 sw=4 et
