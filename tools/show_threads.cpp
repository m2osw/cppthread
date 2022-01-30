// Copyright (c) 2020-2022  Made to Order Software Corp.  All Rights Reserved
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


// last include
//
#include    <snapdev/poison.h>



int main(int argc, char * argv[])
{
    for(int i(1); i < argc; ++i)
    {
        cppthread::process_ids_t pids(cppthread::is_process_running(atoi(argv[i])));
        for(auto p : pids)
        {
            std::cout << p << " ";
        }
        std::cout << "\n";
    }

    return 0;
}

// vim: ts=4 sw=4 et
