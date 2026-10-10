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
 * \brief A guard to lock a region of code.
 *
 * This file includes the declaration if the guard object used to
 * safely lock a mutex or a rwlock.
 *
 * It is safe because it allows your code to raise exceptions and
 * still get the expected unlock called.
 */



namespace cppthread
{


class mutex;
class recursive_rwlock;
class rwlock;


class guard
{
public:
                        guard(mutex & m);
                        guard(recursive_rwlock & r, bool exclusive = true);
                        guard(rwlock & l, bool exclusive = true);
                        guard(guard const & rhs) = delete;
                        ~guard();

    guard &             operator = (guard const & rhs) = delete;

    void                unlock(bool done = true);
    void                lock();
    bool                is_locked() const;

private:
    bool                f_locked = false;
    bool                f_exclusive = false;
    mutex *             f_mutex = nullptr;
    recursive_rwlock *  f_recursive_rwlock = nullptr;
    rwlock *            f_rwlock = nullptr;
};



} // namespace cppthread
// vim: ts=4 sw=4 et
