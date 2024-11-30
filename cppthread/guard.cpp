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
 * exceptions. It is extremely important to unlock all mutexes before
 * a thread quits otherwise the application will lock up.
 *
 * \code
 *    {
 *        cppthread::guard lock(my_mutex);
 *        ... // atomic work
 *    }
 * \endcode
 *
 * \warning
 * This guard implementation assumes that the guard itself is used in one
 * single thread. In other words, even though you could add a guard inside
 * an object as a variable member, the lock() and is_locked() functions are
 * not safe in that situation. The constructor and destructor are, so you
 * could still do it that way. It is still preferred to push a guard on
 * the stack as shown in the example above, rather than as a variable member
 * of a class you then create on the stack.
 */






/** \brief Lock a mutex.
 *
 * This function locks the specified mutex and keep track of the lock
 * until the destructor is called.
 *
 * The mutex parameter cannot be a reference to a nullptr pointer.
 *
 * \param[in] m  The Snap! mutex to lock.
 */
guard::guard(mutex & m)
    : f_mutex(&m)
{
    if(f_mutex == nullptr)
    {
        // mutex is mandatory
        //
        throw logic_error("mutex missing in guard() constructor");
    }
    f_mutex->lock();
    f_locked = true;
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
            << "mutex::unlock() threw an exception while in the ~guard() function."
            << end;
        std::terminate();
    }
}


/** \brief Unlock this mutex.
 *
 * This function can be called any number of times. If the mutex is currently
 * locked, the function unlocks it, otherwise nothing happens.
 *
 * If necessary, you can relock the mutex using the lock() function.
 *
 * This function may throw an exception if the mutex::unlock() call fails.
 *
 * \param[in] done  Whether you are done with this guard, if so, the pointer
 * will be set to null and you can then destroy the mutex (this is the
 * default). If instead you want to be able to re-lock the mutex, then set
 * this parameter to false. The mutex cannot be destroyed if this parameter
 * is set to false.
 *
 * \sa lock()
 */
void guard::unlock(bool done)
{
    if(f_locked)
    {
        mutex * m(f_mutex);
        f_locked = false;
        if(done)
        {
            f_mutex = nullptr;
        }
        m->unlock();
    }
}


/** \brief Relock this mutex.
 *
 * This function can be called any number of times. If called while the
 * mutex is not locked, then it gets relocked, otherwise nothing happens.
 *
 * Note that when creating the guard, the mutex is automatically locked,
 * so you rarely need to call this function.
 *
 * This is most often used when a mutex needs to be unlocked within a
 * guarded block:
 *
 * \code
 *     {
 *         cppthread::guard lock(f_mutex);
 *
 *         ...do some things...
 *
 *         if(this_or_that)
 *         {
 *             lock.unlock();
 *             do_special_thing_while_unlocked();
 *             lock.lock();
 *         }
 *
 *         ...do more things...
 *     }
 * \endcode
 *
 * This example shows how one can run do_special_thing_while_unlocked()
 * while the lock is not being held. The way it is written is still
 * RAII safe. If the do_special_thing_while_unlocked() throws, the mutex
 * is in a known state (i.e. unlocked when exiting the guarded block).
 *
 * \note
 * This implementation always attempts a mutex::lock(), checks whether it
 * was necessary (i.e. is the f_locked flag false?) and if not, it unlocks
 * the mutex (since the guard already had the lock to itself).
 *
 * \warning
 * It is not 100% safe to call this function when the guard::unlock()
 * function is called with its done parameter set to true, which is the
 * default if you don't specify false in your call. It is safe if the
 * guard is only used on the stack.
 *
 * \sa unlock()
 */
void guard::lock()
{
    if(f_mutex == nullptr)
    {
        return;
    }

    f_mutex->lock();

    if(f_locked)
    {
        f_mutex->unlock();
    }
    else
    {
        f_locked = true;
    }
}


/** \brief This function returns whether the guard is current locked.
 *
 * This function returns the f_locked flag of the guard object. If true,
 * then the guard is expected to have its mutex locked. If false, then
 * the guard mutex is not currently locked.
 *
 * \warning
 * This function is not 100% safe if you call unlock() with its done
 * parameter set to true, which is the default. It is safe if the guard
 * is only used by on the stack.
 *
 * \return true if the guard currently holds the lock.
 */
bool guard::is_locked() const
{
    bool result(f_mutex != nullptr);
    if(result)
    {
        f_mutex->lock();
        result = f_locked;
        f_mutex->unlock();
    }

    return result;
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


/** \var guard::f_locked
 * \brief Whether the guard is currently in effect.
 *
 * This flag is used to know whether the mutex is currently considered locked
 * by the guard object.
 */



} // namespace cppthread
// vim: ts=4 sw=4 et
