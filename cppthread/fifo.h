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


// snapdev lib
//
#include    <snapdev/not_used.h>


// C++ lib
//
#include    <numeric>
#include    <deque>



namespace cppthread
{



template<class T>
class fifo
    : public mutex
{
private:
    typedef std::deque<T>   items_t;

    // void_t is C++17 so to compile on more systems, we have our own definition
    //
    template<typename ...> using void_t = void;


    // the following templates are used to know whether class T has a
    // valid_workload() function returning a bool and if so, we'll use
    // it to know whether an item is ready to be popped.
    //
    template<typename, typename, typename = void_t<>>
        struct item_has_predicate
            : public std::false_type
    {
    };

    template<typename C, typename R, typename... A>
        struct item_has_predicate<C, R(A...),
                void_t<decltype(std::declval<C>().valid_workload(std::declval<A>()...))>>
            : public std::is_same<decltype(std::declval<C>().valid_workload(std::declval<A>()...)), R>
    {
    };

    template<typename C>
        struct is_shared_ptr
            : std::false_type
    {
    };

    template<typename C>
        struct is_shared_ptr<std::shared_ptr<C>>
            : std::true_type
    {
    };

    /** \brief Validate item.
     *
     * This function checks whether the T::valid_workload() function
     * says the item can be processed now or not.
     *
     * In this case, the class C is not a shared pointer.
     *
     * \tparam C  The type of the item.
     * \param[in] item  The item to verify.
     *
     * \return true if the valid_workload() returns true, false otherwise.
     */
    template<typename C>
    typename std::enable_if<!is_shared_ptr<C>::value
                        && item_has_predicate<C, bool()>::value
                , bool>::type
        validate_item(C const & item)
    {
        return item.valid_workload();
    }

    /** \brief Validate item.
     *
     * This function always returns true. It is used when the item does
     * not have a valid_workload() function defined.
     *
     * \tparam C  The type of the item.
     * \param[in] item  The item to verify.
     *
     * \return Always true.
     */
    template<typename C>
    typename std::enable_if<!is_shared_ptr<C>::value
                         && !item_has_predicate<C, bool()>::value
                , bool>::type
        validate_item(C const & item)
    {
        snapdev::NOT_USED(item);
        return true;
    }

    /** \brief Validate item.
     *
     * This function checks whether the T::valid_workload() function
     * says the item can be processed now or not.
     *
     * In this case, the class C is a shared pointer to an item T.
     *
     * \tparam C  The type of the item.
     * \param[in] item  The item to verify.
     *
     * \return Always true.
     */
    template<typename C>
    typename std::enable_if<is_shared_ptr<C>::value
                        && item_has_predicate<typename C::element_type, bool()>::value
                , bool>::type
        validate_item(C const & item)
    {
        return item->valid_workload();
    }

    /** \brief Validate item.
     *
     * This function always returns true. It is used when the item is a
     * shared pointer and does not have a valid_workload() function defined.
     *
     * \tparam C  The type of the item.
     * \param[in] item  The item to verify.
     *
     * \return Always true.
     */
    template<typename C>
    typename std::enable_if<is_shared_ptr<C>::value
                         && !item_has_predicate<typename C::element_type, bool()>::value
                , bool>::type
        validate_item(C const & item)
    {
        snapdev::NOT_USED(item);
        return true;
    }

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
        f_queue.push_back(v);
        signal();
        return true;
    }

    bool pop_front(T & v, int64_t const usecs)
    {
        guard lock(*this);

        auto cleanup = [&]()
            {
                if(f_done && !f_broadcast && f_queue.empty())
                {
                    // make sure all the threads wake up on this new
                    // "queue is empty" status
                    //
                    broadcast();
                    f_broadcast = true;
                }
            };

        for(;;)
        {
            // search for an item we can pop now
            //
            for(auto it(f_queue.begin()); it != f_queue.end(); ++it)
            {
                bool const result(validate_item<T>(*it));
                if(result)
                {
                    v = *it;
                    f_queue.erase(it);
                    cleanup();
                    return true;
                }
            }

            if(f_done)
            {
                break;
            }

            // when no items can be returned, wait a bit if possible
            // and try again
            //
            if(usecs == -1)
            {
                // wait until signal() wakes us up
                //
                wait();
            }
            else if(usecs > 0)
            {
                if(!timed_wait(usecs))
                {
                    break;
                }
            }
            else // if(usecs == 0)
            {
                // do not wait
                //
                break;
            }
        }
        cleanup();
        return false;
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
