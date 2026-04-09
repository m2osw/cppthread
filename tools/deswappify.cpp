// Copyright (c) 2026  Made to Order Software Corp.  All Rights Reserved
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

// cppthread
//
#include    <cppthread/thread.h>


// libexcept
//
#include    <libexcept/file_inheritance.h>


// snapdev
//
#include    <snapdev/glob_to_list.h>


// C++
//
#include    <cstring>
#include    <iostream>
#include    <string>


// last include
//
#include    <snapdev/poison.h>



void usage(char const * argv0)
{
    std::cout << "Usage: sudo " << argv0 << " [--opts]" << std::endl;
    std::cout << "where --opts is one of:" << std::endl;
    std::cout << "  --all           deswappify all the running processes (not recommended)" << std::endl;
    std::cout << "  --help | -h     print out this help screen" << std::endl;
    std::cout << "  --pid <pid>     the PID of the process to deswappify" << std::endl;
}


int main(int argc, char * argv[])
{
    libexcept::verify_inherited_files();

    pid_t pid(0);
    for(int i(1); i < argc; ++i)
    {
        if(strcmp(argv[i], "--help") == 0
        || strcmp(argv[i], "-h") == 0)
        {
            usage(argv[0]);
            return 3;
        }
        else if(strcmp(argv[i], "--pid") == 0)
        {
            ++i;
            if(i >= argc)
            {
                std::cerr << argv[0] << ":error: pid must be followed by a process identifier." << std::endl;
                return 1;
            }
            pid = atol(argv[i]);
            if(pid <= 0)
            {
                std::cerr << argv[0] << ":error: invalid pid in \"" << argv[i] << "\"." << std::endl;
                return 1;
            }
        }
        else if(strcmp(argv[i], "--all") == 0)
        {
            pid = -1;
        }
        else
        {
            std::cerr << argv[0] << ":error: unexpected command line option \"" << argv[i] << "\"." << std::endl;
            return 1;
        }
    }

    if((pid > 0 || pid == -1)
    && getuid() != 0)
    {
        std::cerr << argv[0] << ":warning: deswappifying generally requires you to be root. If it doesn't work, try again with sudo even if you own the process." << std::endl;
    }

    if(pid > 0)
    {
        errno = cppthread::deswappify(pid);
        return errno == 0 ? 0 : 1;
    }
    else if(pid == -1)
    {
        snapdev::glob_to_list<std::list<std::string>> glob;
        if(!glob.read_path<
                snapdev::glob_to_list_flag_t::GLOB_FLAG_IGNORE_ERRORS,
                snapdev::glob_to_list_flag_t::GLOB_FLAG_ONLY_DIRECTORIES>("/proc/*"))
        {
            std::cerr << argv[0] << ":error: could not read /proc for a list of processes." << std::endl;
            return 1;
        }
        int e(0);
        for(auto const & p : glob)
        {
            pid = atol(p.c_str());
            if(pid == -1)
            {
                // not a valid number, skip
                //
                continue;
            }
            errno = cppthread::deswappify(pid);
            if(errno != 0)
            {
                e = 1;
            }
        }
        return e;
    }
    else
    {
        std::cerr << argv[0] << ":error: nothing to do. Try again with --pid or --all." << std::endl;
    }

    return 1;
}

// vim: ts=4 sw=4 et
