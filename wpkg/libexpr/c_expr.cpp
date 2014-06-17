/*    c_expr.c++ -- a sample usage, also a way to test the library
 *    Copyright (C) 2007-2013  Made to Order Software Corporation
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License along
 *    with this program; if not, write to the Free Software Foundation, Inc.,
 *    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *    Authors
 *    Alexis Wilke   alexis@m2osw.com
 */

/** \file
 * \brief Command line tool that execute a C-like expression.
 *
 * This file is the implementation of a console program that can be used
 * to compute C-like expressions. This is similar to the expr Unix command
 * except that you do not need spaces around operators and all the C
 * operators are available. Note, however, that you probably will want to
 * write your expressions between quotes.
 *
 * For example:
 *
 * \code
 * c-expr '3 + 4 * 7'
 * \endcode
 *
 * outputs:
 *
 * \code
 * Expression "3 + 4 * 7" evaluates to: 31
 * \endcode
 *
 * The command accepts any number of expressions on the command line.
 */
#include	"libexpr/expr.h"

#include	<iostream>
#include	<string.h>
#include	<stdio.h>
#include	<stdlib.h>

int main(int argc, char *argv[])
{
	int	i;

	if(argc == 2)
	{
		if(strcmp(argv[1], "--version") == 0)
		{
			printf("%s\n", LIBEXPR_VERSION_STRING);
			exit(1);
		}
		if(strcmp("-h", argv[1]) == 0 || strcmp("--help", argv[1]) == 0 || strcmp("--help-nobr", argv[1]) == 0)
		{
		    printf("Usage: c_expr [options] <c-like-expression>\n");
		    printf("Where options may be one or more of the following:\n");
		    printf("  --help or -h     prints out this help screen\n");
		    printf("  --version        prints out the version information\n");
		    exit(1);
		}
	}

	i = 1;
	while(i < argc)
	{
		libexpr::expr_evaluator e;
		libexpr::variable result;
		e.eval(argv[i], result);
		std::string dump;
		result.to_string(dump);
		std::cout << "Expression \"" << argv[i] << "\" evaluates to: " << dump << std::endl;
		++i;
	}

	return 0;
}

// vim: ts=8 sw=8
