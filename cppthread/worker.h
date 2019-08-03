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
#include    "cppthread/fifo.h"
#include    "cppthread/runner.h"



namespace cppthread
{



template<class T>
class worker
    : public runner
{
public:
    typedef T       work_load_type;

    worker<T>(std::string const & name
            , size_t position
            , typename fifo<T>::pointer_t in
            , typename fifo<T>::pointer_t out)
        : runner(name)
        , f_in(in)
        , f_out(out)
        , f_position(position)
    {
    }

    worker<T>(worker const & rhs) = delete;
    worker<T> & operator = (worker<T> const & rhs) = delete;

    size_t position() const
    {
        return f_position;
    }

    bool is_working() const
    {
        guard lock(f_mutex);
        return f_working;
    }

    size_t runs() const
    {
        guard lock(f_mutex);
        return f_runs;
    }

    virtual void run()
    {
        // on a re-run, f_working could be true
        {
            guard lock(f_mutex);
            f_working = false;
        }

        while(continue_running())
        {
            if(f_in->pop_front(f_workload, -1))
            {
                if(continue_running())
                {
                    {
                        guard lock(f_mutex);
                        f_working = true;
                        ++f_runs;
                    }

                    // note: if do_work() throws, then f_working remains
                    //       set to 'true' which should not matter
                    //
                    if(do_work())
                    {
                        f_out->push_back(f_workload);
                    }

                    {
                        guard lock(f_mutex);
                        f_working = false;
                    }
                }
            }
            else
            {
                // if the FIFO is empty and it is marked as done, we
                // want to exit immediately
                //
                if(f_in->is_done())
                {
                    break;
                }
            }
        }
    }

    virtual bool do_work() = 0;

protected:
    T                       f_workload = T();
    typename fifo<T>::pointer_t f_in;
    typename fifo<T>::pointer_t f_out;

private:
    size_t const            f_position;
    bool                    f_working = false;
    size_t                  f_runs = 0;
};



} // namespace cppthread
// vim: ts=4 sw=4 et
