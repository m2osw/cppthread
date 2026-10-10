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
 * \brief A recursive read-only or read-write lock.
 *
 * This file includes the declaration of the C++ Thread `recursive_rwlock`.
 * This is used for synchronization between threads by locking access
 * either in read-only mode (shared) or read-write mode (exclusive).
 *
 * This version allows recursivity, meaning that a thread can call the
 * lock()/rlock() functions any number of times.
 *
 * Even though this class allows recursivity, it still prioritize
 * readers by default. Make sure to set the flag to false when
 * constructing your recursive_rwlock object if you want to prioritize
 * writers.
 *
 * It is not copyable.
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
class recursive_rwlock_impl;
}


class recursive_rwlock
{
public:
    typedef std::shared_ptr<recursive_rwlock>   pointer_t;
    typedef std::vector<pointer_t>              vector_t;

                        recursive_rwlock(bool prefer_reader = true);
                        recursive_rwlock(recursive_rwlock const & rhs) = delete;
                        ~recursive_rwlock();

    recursive_rwlock &  operator = (recursive_rwlock const & rhs) = delete;

    bool                is_locked() const;
    bool                is_exclusively_locked() const;
    bool                is_shared_locked() const;

    bool                own_lock() const;
    void                lock();
    bool                try_lock();
    bool                timed_lock(timespec const & wait);
    bool                dated_lock(timespec const & date);

    bool                own_rlock() const;
    void                rlock();
    bool                try_rlock();
    bool                timed_rlock(timespec const & wait);
    bool                dated_rlock(timespec const & date);

    void                unlock();

private:
    std::shared_ptr<detail::recursive_rwlock_impl>
                        f_impl;
};



} // namespace cppthread
// vim: ts=4 sw=4 et
