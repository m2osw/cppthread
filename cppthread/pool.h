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
#include    "cppthread/exception.h"
#include    "cppthread/fifo.h"
#include    "cppthread/thread.h"



namespace cppthread
{



template<class W, class ...A>
class pool
{
public:
    typedef std::shared_ptr<pool<W, A...>>      pointer_t;
    typedef typename W::work_load_type          work_load_type;
    typedef fifo<work_load_type>                worker_fifo_t;

private:
    class worker_thread_t
    {
    public:
        typedef std::shared_ptr<worker_thread_t>    pointer_t;
        typedef std::vector<pointer_t>              vector_t;

        worker_thread_t(std::string const & name
                      , size_t i
                      , typename worker_fifo_t::pointer_t in
                      , typename worker_fifo_t::pointer_t out
                      , A... args)
            : f_worker(name + " (worker #" + std::to_string(i) + ")"
                     , i
                     , in
                     , out
                     , args...)
            , f_thread(std::make_shared<thread>(name, &f_worker))
        {
            f_thread->start();
        }

        W & get_worker()
        {
            return f_worker;
        }

        W const & get_worker() const
        {
            return f_worker;
        }

    private:
        W                       f_worker;   // runner before thread; this is safe
        thread::pointer_t       f_thread;
    };


public:
    pool<W, A...>(std::string const & name
                         , size_t pool_size
                         , typename worker_fifo_t::pointer_t in
                         , typename worker_fifo_t::pointer_t out
                         , A... args)
        : f_name(name)
        , f_in(in)
        , f_out(out)
    {
        if(pool_size == 0)
        {
            throw cppthread_exception_invalid_error("the pool size must be a positive number (1 or more)");
        }
        if(pool_size > 1000)
        {
            throw cppthread_exception_invalid_error("pool size too large (we accept up to 1000 at this time, which is already very very large!)");
        }
        for(size_t i(0); i < pool_size; ++i)
        {
            f_workers.push_back(std::make_shared<worker_thread_t>(
                          f_name
                        , i
                        , f_in
                        , f_out
                        , args...));
        }
    }

    ~pool<W, A...>()
    {
        stop(false);
        wait();
    }

    size_t size() const
    {
        return f_workers.size();
    }

    W & get_worker(int i)
    {
        if(static_cast<size_t>(i) >= f_workers.size())
        {
            throw std::range_error("snap::thread::pool::get_worker() called with an index out of bounds.");
        }
        return f_workers[i]->get_worker();
    }

    W const & get_worker(int i) const
    {
        if(static_cast<size_t>(i) >= f_workers.size())
        {
            throw std::range_error("snap::thread::pool::get_worker() called with an index out of bounds.");
        }
        return f_workers[i]->get_worker();
    }

    void push_back(work_load_type const & v)
    {
        f_in.push_back(v);
    }

    bool pop_front(work_load_type & v, int64_t usecs)
    {
        if(f_in.is_done())
        {
            usecs = 0;
        }
        return f_out.pop_front(v, usecs);
    }

    void stop(bool immediate)
    {
        if(!f_in->is_done())
        {
            f_in->done(immediate);
        }
    }

    void wait()
    {
        f_workers.clear();
    }


private:
    typedef typename worker_thread_t::vector_t  workers_t;

    std::string const                   f_name;
    typename worker_fifo_t::pointer_t   f_in;
    typename worker_fifo_t::pointer_t   f_out;
    workers_t                           f_workers = workers_t();
};



} // namespace cppthread
// vim: ts=4 sw=4 et
