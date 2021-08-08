// Copyright (c) 2013-2021  Made to Order Software Corp.  All Rights Reserved
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


// C++ lib
//
#include    <memory>
#include    <vector>



namespace cppthread
{



namespace detail
{
class mutex_impl;
}


// a mutex to ensure single threaded work
//
class mutex
{
public:
    typedef std::shared_ptr<mutex>     pointer_t;
    typedef std::vector<pointer_t>     vector_t;
    typedef std::vector<mutex>         direct_vector_t;

                        mutex();
                        ~mutex();

    void                lock();
    bool                try_lock();
    void                unlock();
    void                wait();
    bool                timed_wait(uint64_t const usec);
    bool                dated_wait(uint64_t const usec);
    void                signal();
    void                safe_signal();
    void                broadcast();
    void                safe_broadcast();

private:
    std::shared_ptr<detail::mutex_impl>
                        f_impl;

    uint32_t            f_reference_count = 0;
};



extern mutex *          g_system_mutex;



} // namespace cppthread
// vim: ts=4 sw=4 et
