// Copyright (c) 2013-2026  Made to Order Software Corp.  All Rights Reserved
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
#pragma once

/** \file
 * \brief Mutex lock.
 *
 * This file includes the declaration of the C++ Thread `mutex`. This is
 * used for synchronization between threads by locking access.
 *
 * This is a recursive mutex. It is not copyable. You can, however, use it
 * in a shared pointer which can be copied (all pointers will, of course,
 * point to the same mutex).
 */


// C++
//
#include    <atomic>
#include    <cstdint>
#include    <memory>
#include    <vector>



namespace cppthread
{



namespace detail
{
class mutex_impl;
}


class mutex
{
public:
    typedef std::shared_ptr<mutex>     pointer_t;
    typedef std::vector<pointer_t>     vector_t;
    typedef std::vector<mutex>         direct_vector_t;

                        mutex();
                        mutex(mutex const & rhs) = delete;
                        ~mutex();

    mutex &             operator = (mutex const & rhs) = delete;

    void                lock();
    bool                try_lock();
    void                unlock();
    void                wait();
    bool                timed_wait(std::uint64_t const usec);
    bool                timed_wait(timespec const & nsec);
    bool                dated_wait(std::uint64_t const date);
    bool                dated_wait(timespec const & date);
    void                signal();
    void                safe_signal();
    void                broadcast();
    void                safe_broadcast();

private:
    void                is_locked_once();

    std::shared_ptr<detail::mutex_impl>
                        f_impl;

    std::atomic<std::uint32_t>
                        f_reference_count = 0;
};


extern mutex *          g_system_mutex;



} // namespace cppthread
// vim: ts=4 sw=4 et
