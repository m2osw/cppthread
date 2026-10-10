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
 * \brief A read-only or read-write lock.
 *
 * This file includes the declaration of the C++ Thread `rwlock`. This is
 * used for synchronization between threads by locking access either in
 * read-only mode (shared) or read-write mode (exclusive).
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
class rwlock_impl;
}


class rwlock
{
public:
    typedef std::shared_ptr<rwlock>    pointer_t;
    typedef std::vector<pointer_t>     vector_t;

                        rwlock(bool prefer_reader = true);
                        rwlock(rwlock const & rhs) = delete;
                        ~rwlock();

    rwlock &             operator = (rwlock const & rhs) = delete;

    void                lock();
    bool                try_lock();
    bool                timed_lock(timespec const & wait);
    bool                dated_lock(timespec const & date);

    void                rlock();
    bool                try_rlock();
    bool                timed_rlock(timespec const & wait);
    bool                dated_rlock(timespec const & date);

    void                unlock();

private:
    std::shared_ptr<detail::rwlock_impl>        f_impl;
};



} // namespace cppthread
// vim: ts=4 sw=4 et
