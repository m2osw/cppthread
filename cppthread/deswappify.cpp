// Copyright (c) 2026  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/cppthread
// contact@m2osw.com
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

/** \file
 * \brief Implementation of the deswappify function.
 *
 * It is possible to ask the kernel to reload part of the swapped out
 * processes. This is done by loading that section of memory (with a
 * simple read() call).
 *
 * This is based on a perl script found on github:
 *
 * https://gist.github.com/WGH-/91260f6d65db88be2c847053c49be5ae
 */


// self
//
#include    "cppthread/thread.h"



// snapdev
//
#include    <snapdev/glob_to_list.h>
#include    <snapdev/hexadecimal_string.h>


// C++
//
#include    <fstream>
#include    <sstream>


// last include
//
#include    <snapdev/poison.h>




namespace cppthread
{





/** \brief Go through the swapped out sections of a process and deswappify them.
 *
 * This function reads the memory map of a process and searches for sections
 * of memory that were swapped out and read them back in memory.
 *
 * \todo
 * Implement necessary check to make sure the data can be swapped back in
 * without causing immediate issues.
 *
 * \todo
 * Actually write a function that reads all the blocks and save them in
 * a list with a grand total for the size. Then we can make the decision
 * of swapping that process back in or not.
 *
 * \param[in] pid  The process identifier.
 *
 * \return 0 if no error occurred, an errno otherwise
 */
int deswappify(pid_t pid)
{
    std::stringstream smaps;
    smaps << "/proc/" << pid << "/smaps";

    std::stringstream mem;
    mem << "/proc/" << pid << "/mem";

    try
    {
        std::ifstream in(smaps.str());
        std::ifstream mb(mem.str());

        std::int64_t start(0);
        std::int64_t end(0);
        while(in)
        {
            std::string l;
            std::getline(in, l);
            if(l.empty())
            {
                break;
            }

            // each block is separated by one line with two hexadecimal
            // numbers (addresses) separated by a dash:
            //
            //    start-end <flags> ...
            //
            // if not valid, then try the next possibility which is
            // a line starting with "Swap:"
            //
            char const * s(l.c_str());

            std::int64_t new_start(0);
            for(; snapdev::is_hexdigit(*s); ++s)
            {
                new_start *= 16;
                new_start += snapdev::hexdigit_to_number(*s);
            }
            if(*s == '-')
            {
                // doing good so far...
                //
                end = 0;
                for(++s; snapdev::is_hexdigit(*s); ++s)
                {
                    end *= 16;
                    end += snapdev::hexdigit_to_number(*s);
                }
                if(new_start != 0
                && end != 0)
                {
                    // assume start-end ... found
                    //
                    start = new_start;
                    continue;
                }
                else
                {
                    end = 0;
                }
            }
            if(start == 0
            || end == 0)
            {
                continue;
            }

            if(*s == 'S'
            && s[1] == 'w'
            && s[2] == 'a'
            && s[3] == 'p'
            && s[4] == ':')
            {
                s += 6;
                while(isspace(*s))
                {
                    ++s;
                }
                int kb(0);
                for(; *s >= '0' && *s <= '9'; ++s)
                {
                    kb *= 10;
                    kb += *s - '0';
                }
                while(isspace(*s))
                {
                    ++s;
                }
                if(kb > 0
                && s[0] == 'k'
                && s[1] == 'B')
                {
                    // found a swapped out memory block, read it
                    //
                    try
                    {
                        mb.seekg(start);
                        char buf[4096]; // TODO: verify page size!
                        for(; start < end; start += 4096)
                        {
                            mb.read(buf, 4096);
                        }
                        start = 0;
                        end = 0;
                    }
                    catch(std::system_error const & e)
                    {
                        return e.code().value();
                    }
                }
                // else -- not a valid Swap: ... entry or not swapped out (0 kB)
            }
            // else -- ignore the other fields
        }
    }
    catch(std::system_error const & e)
    {
        return e.code().value();
    }

    return 0;
}



} // namespace cppthread
// vim: ts=4 sw=4 et
