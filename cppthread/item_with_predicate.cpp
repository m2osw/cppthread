// Copyright (c) 2013-2021  Made to Order Software Corp.  All Rights Reserved
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

/** \file
 * \brief Implementation & documentation of the item_with_predicate.h file.
 *
 * The item_with_predicate.h file is a class which we implement and
 * document here.
 *
 * The class is used to add sets of items in a FIFO of a pool of workers,
 * items which depend on each other. If a dependency is still defined,
 * then the item is skipped (left in the FIFO) until the valid_workload()
 * returns true, which happens once all the dependencies were worked on.
 *
 * \note
 * We consider this base class as an example. Your own version of the
 * valid_workload() function could use all sorts of predicates. For
 * example, a workload may become valid only after a given date and time,
 * when a file appears, etc.
 */


// self
//
#include    "cppthread/item_with_predicate.h"

#include    "cppthread/exception.h"
#include    "cppthread/guard.h"


// C++ lib
//
#include    <iostream>



namespace cppthread
{



/** \class item_with_predicate
 * \brief A runner augmentation allowing for worker threads.
 *
 * This class allows you to create a pool of worker threads. This is
 * useful to add/remove work in a fifo object and have any one
 * worker thread pick up the next load as soon as it becomes
 * available. This is pretty much the fastest way to get work done
 * using threads, however, it really only works if you can easily
 * break down the work by chunk.
 *
 * One pool of worker threads is expected to share one pair of fifo
 * objects. Also. the input and output fifo objects must be of the
 * same type.
 *
 * To use your pool of threads, all you have to do is add data to the
 * input fifo and grab results from the output fifo. Note that
 * the output fifo of one pool of threads can be the input fifo
 * of another pool of threads.
 */


/** \brief Initialize the item with one dependency.
 *
 * This constructor initializes the item with one \p dependency.
 * If you do not yet have the list of dependencies for this item,
 * then you can instead use the add_dependency() and add_dependencies()
 * functions to add them later.
 *
 * As long as you have an item and its processing didn't start yet, you can
 * add additional dependencies.
 *
 * \param[in] dependency  The dependency to add to this item on creation.
 */
item_with_predicate::item_with_predicate(pointer_t dependency)
    : f_dependencies({ dependency })
{
}


/** \brief Initialize the item with a list of dependencies.
 *
 * This constructor initializes the item with a list of dependencies.
 * If you do not yet have the list of dependencies, then you
 * can instead use the add_dependency() or add_dependencies() functions
 * to add dependencies later.
 *
 * As long as you have an item and its processing didn't start yet, you can
 * add additional dependencies.
 *
 * \note
 * Since C++11 we can call this function with a list of items as in:
 *
 * std::make_shared<item_with_predicate>({ a, b, c, d, ... });
 *
 * which makes this constructor particularly practical.
 *
 * \param[in] dependencies  The dependencies to add to this item.
 */
item_with_predicate::item_with_predicate(dependencies_t const & dependencies)
    : f_dependencies(dependencies)
{
}


/** \brief The destructor of the item with predicate.
 *
 * This function is here because the class is virtual and thus a destructor
 * is always required.
 */
item_with_predicate::~item_with_predicate()
{
}


/** \brief Add an item as a predicate of this item.
 *
 * This function adds the specified item as a predicate of this item.
 * This means that predicate item needs to be processed before this
 * item gets processed.
 *
 * The predicate is just another item_with_predicate object. A weak
 * pointer is kept by this item_with_predicate object. When the thread
 * handling the predicate is done, the shared pointer will be released
 * meaning that the weak pointer that this item holds will be released
 * too. When all the dependencies added here are released, the
 * valid_workload() function returns true and this very workload item
 * gets processed.
 *
 * \exception cppthread_in_use_error
 * This exception is raised if this item was already sent to a thread for
 * processing since by then it's too late, you just can't hope to stop
 * the processing or restart it.
 *
 * \todo
 * See whether we could make the predicate any kind of objects with
 * a template?
 *
 * \param[in] item  A predicate item.
 */
void item_with_predicate::add_dependency(pointer_t item)
{
    guard lock(f_mutex);

    if(f_processing)
    {
        throw cppthread_in_use_error("workload already being processed, you can't add more dependencies to it.");
    }

    f_dependencies.push_back(item);
}


/** \brief Add a set of dependencies at once.
 *
 * This function adds all the dependencies found in the \p dependencies
 * parameter to this item_with_predicate object. This is equivalent
 * to adding the dependencies one at a time to this item.
 *
 * \exception cppthread_in_use_error
 * This exception is raised if this item was already sent to a thread for
 * processing since by then it's too late, you just can't hope to stop
 * the processing or restart it.
 *
 * \param[in] dependencies  The dependencies to add to this item.
 */
void item_with_predicate::add_dependencies(dependencies_t const & dependencies)
{
    guard lock(f_mutex);

    if(f_processing)
    {
        throw cppthread_in_use_error("workload already being processed, you can't add more dependencies to it.");
    }

    f_dependencies.insert(f_dependencies.begin(), dependencies.cbegin(), dependencies.cend());
}


/** \brief The valid_workload() to test whether we can process this item.
 *
 * When working with a thread pool, you add workload items to the FIFO
 * and they get executed in order unless you have a valid_workload()
 * function. In that case you have to run the process of an item added
 * to the FIFO only if:
 *
 * * There is a thread available,
 * * The valid_workload() returns true.
 *
 * If you want additional tests, you can overload the function since
 * it's a virtual function.
 *
 * \warning
 * A side effect of calling this function is to mark the item as being
 * processed. At that point, further adding of dependencies is not
 * possible.
 *
 * \return true if the item is ready to be processed (i.e. all of its
 * dependencies were processed).
 */
bool item_with_predicate::valid_workload() const
{
    guard lock(f_mutex);

    for(auto it(f_dependencies.begin()); it != f_dependencies.end(); )
    {
        if(it->expired())
        {
            it = f_dependencies.erase(it);
        }
        else
        {
            ++it;
        }
    }

    if(f_dependencies.empty())
    {
        f_processing = true;
        return true;
    }

    return false;
}





/** \typedef item_with_predicate::pointer_t
 * \brief The item_with_predicate shared pointer type.
 *
 * To use an item_with_predicate, we strongly advice that you use a shared
 * pointer. This type defines that shared pointer.
 */


/** \typedef item_with_predicate::weak_pointer_t
 * \brief The item_with_predicate weak pointer type.
 *
 * The items are added as dependencies and in that case we add them as
 * weak pointers so when done with a workload, it \em disappears
 * automatically and the predicate becomes true.
 */


/** \typedef item_with_predicate::dependencies_t
 * \brief The type representing the list of dependencies.
 *
 * This type is used to hold the list of dependencies as weak pointers.
 * Once that list is empty (we automatically remove pointers which can't
 * be locked anymore), the predicate is considered true and the this
 * workload can then be worked on by a thread from the pool.
 */



/** \var item_with_predicate::f_mutex
 * \brief The mutex used to protect the predicate variables.
 *
 * This mutex is used to make sure that functions that modify the
 * variable members do so safely (i.e. only one thread at a time).
 */


/** \var item_with_predicate::f_dependencies
 * \brief Set of dependencies.
 *
 * This parameter holds a set of _dependencies_, which is a set of other
 * items which have to be fully processed before this item can be
 * processed.
 *
 * The fifo::pop_front() function calls the valid_workload() to know
 * whether the item has dependencies. It becomes true only once
 * all the dependencies were processed.
 *
 * The set uses weak pointers and detects that another item processing
 * is done because it gets released. In your code, you must make sure
 * that all the item allocations you've made go out of scope by the time
 * you start the execution, otherwise valid_workload() will return false
 * forever.
 */


/** \var item_with_predicate::f_processing
 * \brief Whether this workload is being processed.
 *
 * If this workload predicate is true, then it can be processed and thus
 * this flag becomes true. At that point, the add_dependency() and
 * add_dependencies() do not work anymore (if you call them, they
 * raise an exception).
 */




} // namespace cppthread
// vim: ts=4 sw=4 et
