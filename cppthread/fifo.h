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
#include    "cppthread/guard.h"
#include    "cppthread/mutex.h"


// C++ lib
//
#include    <algorithm>
#include    <queue>



namespace cppthread
{



template<class T>
class fifo
    : public mutex
{
private:
    typedef std::queue<T>   items_t;

public:
    typedef T                               value_type;
    typedef fifo<value_type>                fifo_type;
    typedef std::shared_ptr<fifo_type>      pointer_t;

    bool push_back(T const & v)
    {
        guard lock(*this);
        if(f_done)
        {
            return false;
        }
        f_queue.push(v);
        signal();
        return true;
    }

    bool pop_front(T & v, int64_t const usecs)
    {
        guard lock(*this);
        if(!f_done && f_queue.empty())
        {
            // when empty wait a bit if possible and try again
            //
            if(usecs == -1)
            {
                // wait until signal() wakes us up
                //
                wait();
            }
            else if(usecs > 0)
            {
                timed_wait(usecs);
            }
        }
        bool const result(!f_queue.empty());
        if(result)
        {
            v = f_queue.front();
            f_queue.pop();
        }
        if(f_done && !f_broadcast && f_queue.empty())
        {
            // make sure all the threads wake up on this new
            // "queue is empty" status
            //
            broadcast();
            f_broadcast = true;
        }
        return result;
    }

    void clear()
    {
        guard lock(*this);
        items_t empty;
        f_queue.swap(empty);
    }

    bool empty() const
    {
        guard lock(const_cast<fifo &>(*this));
        return f_queue.empty();
    }

    size_t size() const
    {
        guard lock(const_cast<fifo &>(*this));
        return f_queue.size();
    }

    size_t byte_size() const
    {
        guard lock(const_cast<fifo &>(*this));
        return std::accumulate(
                    f_queue.begin(),
                    f_queue.end(),
                    0,
                    [](size_t accum, T const & obj)
                    {
                        return accum + obj.size();
                    });
    }

    void done(bool clear)
    {
        guard lock(*this);
        f_done = true;
        if(clear)
        {
            items_t empty;
            f_queue.swap(empty);
        }
        if(f_queue.empty())
        {
            broadcast();
            f_broadcast = true;
        }
    }

    bool is_done() const
    {
        guard lock(const_cast<fifo &>(*this));
        return f_done;
    }

private:
    items_t                 f_queue = items_t();
    bool                    f_done = false;
    bool                    f_broadcast = false;
};



} // namespace cppthread
// vim: ts=4 sw=4 et
