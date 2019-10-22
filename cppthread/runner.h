// Copyright (c) 2013-2019  Made to Order Software Corp.  All Rights Reserved
// https://snapwebsites.org/project/cppthread
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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#pragma once

/** \file
 * \brief Thread Runner and Managers.
 *
 * This file includes the declaration and implementation (For templates)
 * of classes used to manage threads the easy way. Especially, our
 * implementation is aware of object destructors so a thread manager
 * (snap_thread) can be destroyed. It will automatically and properly
 * wait for its runner (the actual system pthread) to exit before
 * finishing up its and its runner clean up.
 */

// self
//
#include    "cppthread/mutex.h"


// C++ lib
//
#include    <vector>



namespace cppthread
{



class thread;


// this is the actual thread because we cannot use the main thread
// object destructor to properly kill a thread in a C++ environment
//
class runner
{
public:
    typedef std::shared_ptr<runner>         pointer_t;
    typedef std::vector<pointer_t>          vector_t;

                        runner(std::string const & name);
                        runner(runner const & rhs) = delete;
    virtual             ~runner();

    runner &            operator = (runner const & rhs) = delete;

    std::string const & get_name() const;
    virtual bool        is_ready() const;
    virtual bool        continue_running() const;
    virtual void        run() = 0;
    thread *            get_thread() const;
    pid_t               gettid() const;

protected:
    mutable mutex       f_mutex = mutex();

private:
    friend class thread;

    thread *            f_thread = nullptr;
    std::string const   f_name = std::string();
};





} // namespac cppthreade
// vim: ts=4 sw=4 et
