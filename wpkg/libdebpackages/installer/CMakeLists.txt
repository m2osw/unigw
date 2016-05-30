#
# File:
#      libdebpackages/installer/CMakeLists.txt
#
# Description:
#      Files used by the wpkar_installer class.
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2006-2016 Made to Order Software Corp.
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

function( installer_include_files )
	set( LIBDEBPACKAGES_INCLUDE_FILES
		installer/dependencies.h
		installer/flags.h
		installer/install_info.h
		installer/package_item.h
		installer/package_list.h
		installer/progress_scope.h
		installer/task.h
		installer/tree_generator.h
	)

	set( LIBDEBPACKAGES_SOURCE_FILES
		installer/dependencies.cpp
		installer/flags.cpp
		installer/install_info.cpp
		installer/package_item.cpp
		installer/package_list.cpp
		installer/progress_scope.cpp
		installer/task.cpp
		installer/tree_generator.cpp
	)

	if( NOT MO_DARWIN AND NOT MO_SUNOS AND NOT MO_FREEBSD )
		list( APPEND LIBDEBPACKAGES_INCLUDE_FILES installer/details/disk.h   )
		list( APPEND LIBDEBPACKAGES_SOURCE_FILES  installer/details/disk.cpp )
	endif()

	set( INC_FILES ${LIBDEBPACKAGES_INCLUDE_FILES} PARENT_SCOPE )
	set( SRC_FILES ${LIBDEBPACKAGES_SOURCE_FILES}  PARENT_SCOPE )
endfunction()

# vim: ts=4 sw=4 et
