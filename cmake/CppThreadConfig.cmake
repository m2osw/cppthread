# - Try to find CppThread
#
# Once done this will define
#
# CPPTHREAD_FOUND        - System has CppThread
# CPPTHREAD_INCLUDE_DIR  - The CppThread include directories
# CPPTHREAD_LIBRARY      - The libraries needed to use CppThread (none)
# CPPTHREAD_DEFINITIONS  - Compiler switches required for using CppThread (none)
#
# License:
#       Copyright (c) 2013-2019  Made to Order Software Corp.  All Rights Reserved
#
#       https://snapwebsites.org/
#       contact@m2osw.com
# 
#       This program is free software; you can redistribute it and/or modify
#       it under the terms of the GNU General Public License as published by
#       the Free Software Foundation; either version 2 of the License, or
#       (at your option) any later version.
#
#       This program is distributed in the hope that it will be useful,
#       but WITHOUT ANY WARRANTY; without even the implied warranty of
#       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#       GNU General Public License for more details.
#
#       You should have received a copy of the GNU General Public License
#       along with this program; if not, write to the Free Software
#       Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#

find_path( CPPTHREAD_INCLUDE_DIR cppthread/thread.h
		   PATHS $ENV{CPPTHREAD_INCLUDE_DIR}
		 )
find_library( CPPTHREAD_LIBRARY cppthread
		   PATHS $ENV{CPPTHREAD_LIBRARY}
		 )
mark_as_advanced( CPPTHREAD_INCLUDE_DIR CPPTHREAD_LIBRARY )

set( CPPTHREAD_INCLUDE_DIRS ${CPPTHREAD_INCLUDE_DIR} )
set( CPPTHREAD_LIBRARIES    ${CPPTHREAD_LIBRARY}     )

include( FindPackageHandleStandardArgs )
# handle the QUIETLY and REQUIRED arguments and set CPPTHREAD_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args( CppThread DEFAULT_MSG CPPTHREAD_INCLUDE_DIR CPPTHREAD_LIBRARY )
