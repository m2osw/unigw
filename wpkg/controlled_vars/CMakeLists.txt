#
# File:		CMakeLists.txt
# Object:	Definitions to create the build environment
#
# Copyright:	Copyright (c) 2011-2015 Made to Order Software Corp.
#		All Rights Reserved.
#
# http://snapwebsites.org/
# contact@m2osw.com
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

##
## Controlled Variables Generator
##
project(controlled_vars)

include(CheckIncludeFiles)
check_include_files(unistd.h HAVE_UNISTD_H)
if(${CMAKE_SYSTEM_NAME}="Darwin")
  set(OS_DARWIN 1)
endif(${CMAKE_SYSTEM_NAME}="Darwin")

set( CONTROLLED_VARS_MAJOR  1 )
set( CONTROLLED_VARS_MINOR  4 )
set( CONTROLLED_VARS_PATCH 16 )

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/controlled_vars_version.h.in ${CMAKE_CURRENT_BINARY_DIR}/controlled_vars_version.h)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable( ${PROJECT_NAME}
	controlled_vars.cpp
)

##
## Controlled Variables Header
##
project( controlled_vars.h )

if( CMAKE_CROSSCOMPILING )
	set( CMD "${PROJECT_BINARY_DIR}/controlled_vars${CMAKE_EXECUTABLE_SUFFIX}" )
else()
	# Cross-compiling causes this not to work: see above
	set( CMD controlled_vars )
endif()

add_custom_command(
	OUTPUT controlled_vars.h
	COMMAND ${CMD}
	DEPENDS controlled_vars
)

add_custom_target( make_controlled_vars_headers ALL DEPENDS controlled_vars.h )

install(
	DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	DESTINATION include/wpkg
	COMPONENT headers
	FILES_MATCHING PATTERN controlled_vars_*.h
	PATTERN CMakeFiles EXCLUDE
)

