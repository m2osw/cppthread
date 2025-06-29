// Copyright (c) 2013-2025  Made to Order Software Corp.  All Rights Reserved
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
 * \brief Item with Predicate for Worker FIFO Pool.
 *
 * This file defines the base class one can use to have items one can send
 * to a pool of threads for processing. The difference with the default
 * FIFO items, these can include a predicate. The cppthread version
 * uses items as the predicates. So you just have to create all the
 * jobs you want to process as items and then add some of these items as
 * dependencies of other items.
 */

// self
//
#include    <cppthread/mutex.h>


// C++
//
#include    <deque>



namespace cppthread
{



class item_with_predicate
{
public:
    typedef std::shared_ptr<item_with_predicate>
                                pointer_t;
    typedef std::weak_ptr<item_with_predicate>
                                weak_pointer_t;
    typedef std::deque<weak_pointer_t>
                                dependencies_t;

                                item_with_predicate(pointer_t dependency = pointer_t());
                                item_with_predicate(dependencies_t const & dependencies);
    virtual                     ~item_with_predicate();

    void                        add_dependency(pointer_t dependency);
    void                        add_dependencies(dependencies_t const & dependencies);
    virtual bool                valid_workload() const;

private:
    mutable mutex               f_mutex = mutex();
    mutable dependencies_t      f_dependencies = dependencies_t();
    mutable bool                f_processing = false;
};



} // namespace cppthread
// vim: ts=4 sw=4 et
