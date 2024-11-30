// Copyright (c) 2020-2024  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/cppthread
// contact@m2osw.com
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

/** \file
 * \brief Check whether the vDSO is used on this computer.
 *
 * This tool is expected to be used in your scripts to know whether a
 * given process may fail because it is off time wise (i.e. the process
 * uses the time(2) function which can be off by up to 1 second).
 *
 * You can simply use it in an `if` statement like so:
 *
 * \code
 *     if ! has-vdso -q
 *     then
 *         echo "warning: our process won't work right on this platform..."
 *     fi
 * \endcode
 *
 * \note
 * I seem to have such an issue on an ARM64 CPU. So the test above may also
 * require you to test the architecture. I don't recall ever having such an
 * issue on an Intel based computer (I nearly only used computers with
 * Intel processors since I stopped using my SGI computers).
 *
 * \sa https://man7.org/linux/man-pages/man7/vdso.7.html
 */

// cppthread
//
#include    <cppthread/thread.h>


// libexcept
//
#include    <libexcept/file_inheritance.h>


// C++
//
#include    <iostream>


// C
//
#include    <string.h>


// last include
//
#include    <snapdev/poison.h>


void usage(char * progname)
{
    std::cout << "Usage: " << progname << " [-v] [-q] [-h|--help]\n";
    std::cout << "where options are:\n";
    std::cout << "  -v            be more verbose\n";
    std::cout << "  -q            be quiet\n";
    std::cout << "  -h | --help   print out this help screen\n";
}


int main(int argc, char * argv[])
{
    libexcept::verify_inherited_files();

    bool verbose(false);
    bool quiet(false);
    for(int i(1); i < argc; ++i)
    {
        if(strcmp(argv[i], "-v") == 0)
        {
            verbose = true;
        }
        else if(strcmp(argv[i], "-q") == 0)
        {
            quiet = true;
        }
        else if(strcmp(argv[i], "-h") == 0
             || strcmp(argv[i], "--help") == 0)
        {
            usage(argv[0]);
            return 1;
        }
        else
        {
            std::cerr << "error: unsupported command line option \""
                      << argv[i]
                      << "\"."
                      << std::endl;
            return 1;
        }
    }

    if(cppthread::is_using_vdso())
    {
        if(verbose)
        {
            std::cout << "the vDSO is active" << std::endl;
        }
        else if(!quiet)
        {
            std::cout << "true" << std::endl;
        }
        return 0;
    }
    else
    {
        if(verbose)
        {
            std::cout << "no vDSO was detected" << std::endl;
        }
        else if(!quiet)
        {
            std::cout << "false" << std::endl;
        }
        return 1;
    }
}

// vim: ts=4 sw=4 et
