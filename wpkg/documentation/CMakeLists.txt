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
#      Copyright (c) 2013 Made to Order Software Corp.
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

project(wpkg_documentation)

set( GENERATE_DOCS TRUE )

if( MSVC )
    set( PATH_HINT "c:\\cygwin\\bin" )
else()
    set( PATH_HINT "/usr/bin" )
endif()

find_program( RM rm${CMAKE_EXECUTABLE_SUFFIX} PATHS ${PATH_HINT} )
find_program( MV mv${CMAKE_EXECUTABLE_SUFFIX} PATHS ${PATH_HINT} )

if( MSVC )
    set( DOC_FOLDER documentation )
    find_program( ZIP zip${CMAKE_EXECUTABLE_SUFFIX} PATHS ${PATH_HINT} )
    if( ${ZIP} MATCHES "ZIP-NOTFOUND" )
        unset( GENERATE_DOCS )
    endif()
else()
    find_program( TAR_CMD tar${CMAKE_EXECUTABLE_SUFFIX} PATHS ${PATH_HINT} )
    if( ${TAR_CMD} MATCHES "TAR_CMD-NOTFOUND" )
        unset( GENERATE_DOCS )
    endif()
endif()

if( (${RM} MATCHES "RM-NOTFOUND") OR (${MV} MATCHES "MV-NOTFOUND") )
    unset( GENERATE_DOCS )
endif()


################################################################################
# Function AddDoxygenTarget taken from the Snap! environment
# See http://snapwebsites.org/
################################################################################
# Add a target to generate API documentation with Doxygen
#
# Usage: AddDoxygenTarget( TARGET_NAME VERSION_MAJOR VERSION_MINOR VERSION_PATCH )
# where: TARGET_NAME is the name of the project (e.g. libQtCassandra)
#        VERSION_*   shall be used as the base of the tarball generated from the html folder.
#
# AddDoxygenTarget() assumes that the doxyfile lives under CMAKE_CURRENT_SOURCE_DIR, with the name ${TARGET_NAME}.doxy.in.
# The ".in" file must have the INPUT and OUTPUT_DIRECTORY values set appropriately. It is recommended to use @project_SOURCE_DIR@
# for INPUT, where "project" is the actual name of your master project. You can either leave OUTPUT_DIRECTORY empty, or set it to
# @CMAKE_CURRENT_BINARY_DIR@.
#
# Also, for version, use @FULL_VERSION@, which contains the major, minor and patch.
#
function( AddDoxygenTarget TARGET_NAME VERSION_MAJOR VERSION_MINOR VERSION_PATCH )
    if( NOT GENERATE_DOCS )
        return()
    endif()

    project( ${TARGET_NAME}_Documentation )

    set( VERSION "${VERSION_MAJOR}.${VERSION_MINOR}" )
    set( FULL_VERSION "${VERSION}.${VERSION_PATCH}" )

    find_package( Doxygen )

    if( DOXYGEN_FOUND )
        if( NOT DOXYGEN_DOT_FOUND )
            message( WARNING "The dot executable was not found. Did you install Graphviz? No graphic output shall be generated in documentation." )
        endif()

        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${TARGET_NAME}.doxy.in ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.doxy @ONLY)
        set( DOCUMENTATION_OUTPUT ${TARGET_NAME}-doc_${VERSION} )
        
        if( MSVC )
            set( TAR ${ZIP} -r -q )
            set( SUFFIX ".zip" )
        else()
            set( TAR ${TAR_CMD} --exclude='*.md5' -czf )
            set( SUFFIX ".tar.gz" )
        endif()

        add_custom_command( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${DOCUMENTATION_OUTPUT}${SUFFIX} ${DOCUMENTATION_OUTPUT}
            COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.doxy > ${CMAKE_CURRENT_BINARY_DIR}/doxygen.out 2> ${CMAKE_CURRENT_BINARY_DIR}/doxygen.err
            COMMAND echo Compacting as ${DOCUMENTATION_OUTPUT}${SUFFIX}
            COMMAND ${RM} -rf ${DOCUMENTATION_OUTPUT}
            COMMAND ${MV} html ${DOCUMENTATION_OUTPUT}
            COMMAND ${TAR} ${DOCUMENTATION_OUTPUT}${SUFFIX} ${DOCUMENTATION_OUTPUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        )

        add_custom_target( ${TARGET_NAME}_Documentation ALL
            DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${DOCUMENTATION_OUTPUT}${SUFFIX}
            COMMENT "Generating API documentation with Doxygen" VERBATIM
        )

        if( MSVC )
            set_property( TARGET ${TARGET_NAME}_Documentation PROPERTY FOLDER ${DOC_FOLDER} )
        endif()

        # That makes the wpkg-doc package a good 10Mb... I do not think that
        # adds value at this time so I don't do it (although it works.)
        #install(
        #    DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${DOCUMENTATION_OUTPUT}/."
        #    DESTINATION share/doc/wpkg/html
        #    COMPONENT documentation
        #    PATTERN .svn EXCLUDE
        #)
    else()
        message( WARNING "You do not seem to have doxygen installed on this system, no documentation will be generated." )
    endif()
endfunction()

AddDoxygenTarget( wpkg
    ${WPKG_VERSION_MAJOR}
    ${WPKG_VERSION_MINOR}
    ${WPKG_VERSION_PATCH}
)

#if( MSVC )
#    set_property( TARGET wpkg_Documentation PROPERTY FOLDER ${DOCUMENTATION_OUTPUT} )
#endif()

# vim: ts=4 sw=4 et
