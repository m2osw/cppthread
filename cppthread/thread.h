// Snap Websites Server -- advanced handling of Unix thread
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
#include <vector>



namespace cppthread
{


class runner;


class cppthread
{
public:
    typedef std::shared_ptr<cppthread>      pointer_t;
    typedef std::vector<pointer_t>          vector_t;

                                cppthread(std::string const & name, runner * runner);
                                cppthread(cppthread const & rhs) = delete;
                                ~cppthread();

    cppthread &                 operator = (cppthread const & rhs) = delete;

    std::string const &         get_name() const;
    runner *                    get_runner() const;
    bool                        is_running() const;
    bool                        is_stopping() const;
    bool                        start();
    void                        stop();
    bool                        kill(int sig);
    pid_t                       get_thread_tid() const;
    mutex &                     get_thread_mutex() const;

    static int                  get_total_number_of_processors();
    static int                  get_number_of_available_processors();
    static pid_t                gettid();

private:
    // internal function to start the runner
    friend void *               func_internal_start(void * thread);
    void                        internal_run();

    std::string const           f_name = std::string();
    runner *                    f_runner = nullptr;
    mutable mutex               f_mutex = mutex();
    bool                        f_running = false;
    bool                        f_started = false;
    bool                        f_stopping = false;
    pid_t                       f_tid = -1;
    pthread_t                   f_thread_id = -1;
    pthread_attr_t              f_thread_attr = pthread_attr_t();
    std::exception_ptr          f_exception = std::exception_ptr();
};



} // namespace cppthread
// vim: ts=4 sw=4 et
