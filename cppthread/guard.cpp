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
 * \brief Implementation of the Thread Runner and Managers.
 *
 * This file includes the implementation used by the cppthread environment.
 */


// self
//
#include    "cppthread/guard.h"

#include    "cppthread/exception.h"
#include    "cppthread/log.h"
#include    "cppthread/mutex.h"


// last include
//
#include    <snapdev/poison.h>



namespace cppthread
{



/** \class guard
 * \brief Lock a mutex in an RAII manner.
 *
 * This class is used to lock mutexes in a safe manner in regard to
 * exceptions. It is extremely important to lock all mutexes before
 * a thread quits otherwise the application will lock up.
 *
 * \code
 *    {
 *        cppthread::guard lock(my_mutex);
 *        ... // atomic work
 *    }
 * \endcode
 */






/** \brief Lock a mutex.
 *
 * This function locks the specified mutex and keep track of the lock
 * until the destructor is called.
 *
 * The mutex parameter cannot be a reference to a nullptr pointer.
 *
 * \param[in] mutex  The Snap! mutex to lock.
 */
guard::guard(mutex & mutex)
    : f_mutex(&mutex)
{
    if(f_mutex == nullptr)
    {
        // mutex is mandatory
        //
        throw cppthread_logic_error("mutex missing in guard() constructor");
    }
    f_mutex->lock();
}


/** \brief Ensure that the mutex was unlocked.
 *
 * The destructor ensures that the mutex gets unlocked. Note that it is
 * written to avoid exceptions, however, if an exception occurs it ends
 * up calling exit(1).
 *
 * \note
 * If a function throws it logs information using the Snap! logger.
 */
guard::~guard()
{
    try
    {
        unlock();
    }
    catch(std::exception const & e)
    {
        // a log was already printed, we do not absolutely need another one
        log << log_level_t::fatal
            << "guard::unlock() threw an exception while in the ~lock() function."
            << end;
        std::terminate();
    }
}


/** \brief Unlock this mutex.
 *
 * This function can be called any number of times. The first time it is
 * called, the mutex gets unlocked. Further calls (in most cases one more
 * when the destructor is called) have no effects.
 *
 * This function may throw an exception if the mutex unlock call
 * fails.
 */
void guard::unlock()
{
    if(f_mutex != nullptr)
    {
        f_mutex->unlock();
        f_mutex = nullptr;
    }
}




/** \fn guard::guard(guard const & rhs)
 * \brief The copy operator is deleted.
 *
 * The guard class saves a bare pointer to the mutex it is guarding.
 * Because of that, a copy is not really possible and it's also not
 * useful. Plus Effective C++ wants it this way (which is great).
 *
 * \param[in] rhs  The right hand side.
 */


/** \fn guard & guard::operator = (guard const & rhs)
 * \brief The assignment operator is deleted.
 *
 * The guard class saves a bare pointer to the mutex it is guarding.
 * Because of that, an assignment is not really possible and it's also
 * not useful. Plus Effective C++ wants it this way (which is great).
 *
 * \param[in] rhs  The right hand side.
 *
 * \return A reference to this object.
 */



/** \var guard::f_mutex
 * \brief The mutex used by the guard class.
 *
 * Whenever you want to lock a part of your code so only one thread
 * runs it at any given time, you want to use a guard. This guard
 * makes use of a mutex that you pass to it on construction.
 *
 * The guard object keeps a reference to your mutex and uses
 * it to lock on construction and unlock on destruction. This
 * generates a perfect safe guard around your code. Safe guard
 * which is exception safe since it will still get unlocked when
 * an exception occurs.
 *
 * \warning
 * Note that it is not safe if you get a Unix signal. The lock
 * will very likely still be in place if such a signal happens
 * while within the lock.
 */


} // namespace cppthread
// vim: ts=4 sw=4 et
