// Copyright (c) 2013-2024  Made to Order Software Corp.  All Rights Reserved
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
 * \brief Documentation of the fifo.h file.
 *
 * The fifo.h file is a template so we document that template here.
 *
 * The FIFO  is used to push items and pop them as in a FIFO
 * (i.e. first in first out). The newer version of our FIFO
 * has the ability to pop only items that can be popped which
 * allows for better parallelism with a pool of threads.
 */

#error "Documentation only file, do not compile."

namespace cppthread
{



/** \class fifo
 * \brief Create a thread safe FIFO.
 *
 * This template defines a thread safe FIFO which is also a mutex.
 * You should use this snap_fifo object to lock your thread and
 * send messages/data across various threads. The FIFO itself is
 * a mutex so you can use it to lock the threads as with a normal
 * mutex:
 *
 * \code
 *  {
 *      cppthread::snap_lock lock(f_messages);
 *      ...
 *  }
 * \endcode
 *
 * \note
 * It is recommended that you use a smart pointer to your data as
 * the type T. This way you do not have to deal with copies in these
 * FIFOs. However, if your data is very small (a few integers, a
 * small string or two) then you may also use a type T which will
 * be shared by copy. A smart pointer, thou
 *
 * \tparam T  the type of data that the FIFO will handle.
 */

/** \typedef fifo::items_t
 * \brief The container type of our items.
 *
 * All of the FIFO items are pushed and popped from this type of
 * container.
 */

/** \typedef fifo::value_type
 * \brief The type of value to push and pop from the FIFO.
 *
 * This typedef returns the type T of the template.
 */

/** \typedef fifo::fifo_type
 * \brief The type of the FIFO as a typedef.
 *
 * This is a declaration of the FIFO type from the template type T.
 * It can be useful in meta programming.
 */

/** \typedef fifo::pointer_t;
 * \brief A smart pointer to the FIFO.
 *
 * You may want to create FIFOs on the heap in which case we strongly
 * advice that you use this shared pointer type to old those FIFOs.
 *
 * It is otherwise possible to have the FIFO as a variable member of
 * your thread. One thing to consider, though, if that if thread A
 * owns a FIFO and shares it with thread B, then you must make sure
 * that B is done before destroying A.
 */


/** \fn fifo::validate_item(C const & item)
 * \brief Validate a fifo item.
 *
 * The validate_item() checks whether the \p item can be returned by
 * the pop_front() function or not.
 */


/** \fn fifo::validate_item(C const & item)
 * \brief Validate a fifo item.
 *
 * The validate_item() checks whether the \p item can be returned by
 * the pop_front() function or not.
 */


/** \fn fifo::push_back(T const & v)
 * \brief Push data on this FIFO.
 *
 * This function appends data on the FIFO queue. The function
 * has the side effect to wake up another thread if such is
 * currently waiting for data on the same FIFO.
 *
 * \note
 * You can also wake up the other thread by calling the signal()
 * function directly. This is especially useful after you marked
 * the FIFO as done to make sure that all the worker threads
 * wake up and exit cleanly.
 *
 * \attention
 * Remember that if a thread is not currently waiting on the
 * signal, calling signal is not likely to do anything except
 * for the one next thread that waits on that signal.
 *
 * \exception cppthread_exception_invalid_error
 * Do not call this function after calling done(), it will raise
 * this exception if you do so.
 *
 * \param[in] v  The value to be pushed on the FIFO queue.
 *
 * \return true if the value was pushed, false otherwise.
 *
 * \sa done()
 */


/** \fn fifo::pop_front(T & v, int64_t const usecs)
 * \brief Retrieve one value from the FIFO.
 *
 * This function retrieves one value from the thread FIFO.
 * If necessary, the function can wait for a value to be
 * received. The wait works as defined in the semaphore
 * wait() function:
 *
 * \li -1 -- wait forever (use with caution as this prevents
 *           the STOP event from working.)
 * \li 0 -- do not wait if there is no data, return immediately
 * \li +1 and more -- wait that many microseconds
 *
 * If the function works (returns true,) then \p v is set
 * to the value being popped. Otherwise v is not modified
 * and the function returns false.
 *
 * \note
 * Because of the way the pthread conditions are implemented
 * it is possible that the condition was already raised
 * when you call this function. This means the wait, even if
 * you used a value of -1 or 1 or more, will not happen.
 *
 * \note
 * If the function returns false, \p v is not set to anything
 * so it still has the value it had when calling the function.
 *
 * \param[out] v  The value read.
 * \param[in] usecs  The number of microseconds to wait.
 *
 * \return true if a value was popped, false otherwise.
 */

/** \fn fifo::clear()
 * \brief Clear the current FIFO.
 *
 * This function can be used to clear the FIFO. Right after this
 * call, the FIFO will be empty. All the objects that were pushed
 * in the FIFO will be removed. It is your responsibility to ensure
 * they get cleaned up appropriately.
 *
 * \note
 * This function is often used along the done() function to quickly
 * terminate threads.
 *
 * \sa done()
 */

/** \fn fifo::empty() const
 * \brief Test whether the FIFO is empty.
 *
 * This function checks whether the FIFO is empty and if so
 * returns true, otherwise it returns false.
 *
 * The function does not check the semaphore. Instead it
 * checks the size of the FIFO itself.
 *
 * \return true if the FIFO is empty.
 */

/** \fn fifo::size() const
 * \brief Return the number of items in the FIFO.
 *
 * This function returns the number of items currently added to
 * the FIFO. This can be used by the caller to avoid flooding
 * the FIFO, if at all possible.
 *
 * The complexity of this function is O(1).
 *
 * \return the number of items in the FIFO.
 */

/** \fn fifo::byte_size() const
 * \brief Return the total size of the FIFO uses in memory.
 *
 * This function returns the sum of each element size() function.
 *
 * \note
 * This calculation does not include the amount of bytes used by
 * the FIFO itself. It only includes the size of the elements,
 * which in most cases is what you want anyway.
 *
 * The complexity of this function is O(n).
 *
 * \return the byte size of the FIFO.
 */

/** \fn fifo::done(bool clear)
 * \brief Mark the FIFO as done.
 *
 * By default the FIFO is not done. Once you are finished with it
 * and will never push any more data to it, call this function.
 * This flag is used by worker threads to know whether they should
 * wait for more data or just exit.
 *
 * This is rarely used with regular threads. It is more of a feature
 * for worker threads.
 *
 * \note
 * If the FIFO is empty, this function also broadcasts a signal
 * to all the worker threads so that way they can exit.
 *
 * \param[in] clear  Whether the function should also call clear()
 *
 * \sa clear()
 */

/** \fn fifo::is_done() const
 * \brief Check whether the FIFO was marked as done.
 *
 * When a child process calls pop_front() and the function returns
 * false, it means the FIFO is empty. On return, the thread may
 * then check whether is_done() is true. If so, then the thread
 * is expected to exit (no more data will even be added to the
 * FIFO so you might as well leave.)
 *
 * \return true if the thread is expected to exit, false while still
 *         running.
 */

/** \var fifo::f_queue
 * \brief The actual FIFO.
 *
 * This variable member holds the actual data in this FIFO
 * object.
 */

/** \var fifo::f_done
 * \brief Whether the FIFO is done.
 *
 * This flag tells us whether the FIFO is done or not.
 */

/** \var fifo::f_broadcast
 * \brief Whether the done() function called broadcast().
 *
 * This variable is set to true once the done() function called the
 * broadcast() function of the mutex. This way we avoid calling it
 * more than once even if you call the done() function multiple
 * times.
 */



} // namespace cppthread
// vim: ts=4 sw=4 et
