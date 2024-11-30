// Copyright (c) 2013-2022  Made to Order Software Corp.  All Rights Reserved
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
 * \brief Thread Runner Worker.
 *
 * This file includes the declaration and implementation of the worker.
 */

// self
//
#include    <cppthread/fifo.h>
#include    <cppthread/runner.h>



namespace cppthread
{



template<class T>
class worker
    : public runner
{
public:
    typedef T       work_load_type;

    worker(
              std::string const & name
            , std::size_t position
            , typename fifo<T>::pointer_t in
            , typename fifo<T>::pointer_t out)
        : runner(name)
        , f_in(in)
        , f_out(out)
        , f_position(position)
    {
        if(f_in == nullptr)
        {
            throw invalid_error("a worker object must be given a valid input FIFO");
        }
    }

    worker(worker const & rhs) = delete;
    worker<T> & operator = (worker<T> const & rhs) = delete;

    std::size_t position() const
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
                        if(f_out != nullptr)
                        {
                            f_out->push_back(f_workload);
                        }
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
    T                           f_workload = T();
    typename fifo<T>::pointer_t f_in;
    typename fifo<T>::pointer_t f_out;

private:
    std::size_t const           f_position;
    bool                        f_working = false;
    std::size_t                 f_runs = 0;
};



} // namespace cppthread
// vim: ts=4 sw=4 et
