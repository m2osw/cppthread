// Copyright (c) 2020  Made to Order Software Corp.  All Rights Reserved
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

// cppthread lib
//
#include    "cppthread/thread.h"


// C++ lib
//
#include    <iostream>


// C lib
//
#include    <string.h>


// last include
//
#include    <snapdev/poison.h>



int main(int argc, char * argv[])
{
    bool quiet(false);
    bool all(true);
    bool found(false);
    for(int i(1); i < argc; ++i)
    {
        if(strcmp(argv[i], "--help") == 0
        || strcmp(argv[i], "-h") == 0)
        {
            std::cerr << "Usage: " << argv[0] << " [--opts] <pid> ..." << std::endl;
            std::cerr << "where --opts is one of:" << std::endl;
            std::cerr << "  --and | -a      all the processes must exist" << std::endl;
            std::cerr << "  --help | -h     print out this help screen" << std::endl;
            std::cerr << "  --or | -o       at least one of the process must exist" << std::endl;
            std::cerr << "  --quiet | -q    do not generate any output" << std::endl;
            return 3;
        }
        else if(strcmp(argv[i], "--and") == 0
             || strcmp(argv[i], "-a") == 0)
        {
            all = true;
        }
        else if(strcmp(argv[i], "--or") == 0
             || strcmp(argv[i], "-o") == 0)
        {
            all = false;
        }
        else if(strcmp(argv[i], "--quiet") == 0
             || strcmp(argv[i], "-q") == 0)
        {
            quiet = true;
        }
        else
        {
            found = true;
            bool const is_running(cppthread::is_process_running(atoi(argv[i])));
            if(all && !is_running)
            {
                // at least one is not running
                if(!quiet)
                {
                    std::cout << argv[i] << " is not running." << std::endl;
                }
                return 1;
            }
            if(!all && is_running)
            {
                // at least one is running
                if(!quiet)
                {
                    std::cout << argv[i] << " is running." << std::endl;
                }
                return 0;
            }
        }
    }

    if(!found)
    {
        std::cerr << "error: no <pid> where specified." << std::endl;
        return 2;
    }

    if(all)
    {
        if(!quiet)
        {
            std::cout << "all processes are running." << std::endl;
        }
        return 0;
    }

    if(!quiet)
    {
        std::cerr << "none of these processes are running." << std::endl;
    }
    return 1;
}

// vim: ts=4 sw=4 et
