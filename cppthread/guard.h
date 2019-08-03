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



namespace cppthread
{


class mutex;


class guard
{
public:
                        guard(mutex & mutex);
                        guard(guard const & rhs) = delete;
                        ~guard();

    guard &             operator = (guard const & rhs) = delete;

    void                unlock();

private:
    mutex *             f_mutex = nullptr;
};



} // namespace cppthread
// vim: ts=4 sw=4 et
