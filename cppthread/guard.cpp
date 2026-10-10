// Copyright (c) 2013-2026  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/cppthread
// contact@m2osw.com
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

/** \file
 * \brief Implementation of the thread guard.
 *
 * This file implements an RAII way to lock a part of your code using a guard.
 */


// self
//
#include    "cppthread/guard.h"

#include    "cppthread/exception.h"
#include    "cppthread/log.h"
#include    "cppthread/mutex.h"
#include    "cppthread/recursive_rwlock.h"
#include    "cppthread/rwlock.h"


// last include
//
#include    <snapdev/poison.h>



namespace cppthread
{



/** \class guard
 * \brief Manage a lock in an RAII manner.
 *
 * This class is used to lock one of:
 *
 * \li a mutex
 * \li a recursive_rwlock
 * \li a rwlock
 *
 * in a safe manner in regard to exceptions.
 *
 * It is extremely important to unlock all locks before a thread quits
 * otherwise the application is likely to lock up.
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
 * This function locks the specified mutex and keeps track of the lock
 * until the destructor is called.
 *
 * The mutex parameter cannot be a reference to a nullptr pointer.
 *
 * \param[in] m  The mutex to lock.
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


/** \brief Lock a recursive_rwlock.
 *
 * This function locks the specified recursive_rwlock and keeps track
 * of the lock until the destructor is called.
 *
 * The \p l parameter cannot be a reference to a nullptr pointer.
 *
 * \param[in] l  The recursive_rwlock to lock.
 * \param[in] exclusive  Whether to lock exclusive (lock()) or in shared
 *                       mode (rlock()).
 */
guard::guard(recursive_rwlock & l, bool exclusive)
    : f_exclusive(exclusive)
    , f_recursive_rwlock(&l)
{
    if(f_recursive_rwlock == nullptr)
    {
        // recursive_rwlock is mandatory
        //
        throw logic_error("recursive_rwlock missing in guard() constructor");
    }
    if(f_exclusive)
    {
        f_recursive_rwlock->lock();
    }
    else
    {
        f_recursive_rwlock->rlock();
    }
    f_locked = true;
}


/** \brief Lock a rwlock.
 *
 * This function locks the specified rwlock and keeps track of the lock
 * until the destructor is called.
 *
 * The rwlock parameter cannot be a reference to a nullptr pointer.
 *
 * \param[in] l  The rwlock to lock.
 * \param[in] exclusive  Whether to lock exclusive (lock()) or in shared
 *                       mode (rlock()).
 */
guard::guard(rwlock & l, bool exclusive)
    : f_exclusive(exclusive)
    , f_rwlock(&l)
{
    if(f_rwlock == nullptr)
    {
        // rwlock is mandatory
        //
        throw logic_error("rwlock missing in guard() constructor");
    }
    if(f_exclusive)
    {
        f_rwlock->lock();
    }
    else
    {
        f_rwlock->rlock();
    }
    f_locked = true;
}


/** \brief Ensure that the lock is in an unlocked state.
 *
 * The destructor ensures that the lock gets unlocked. Note that
 * it is written to avoid exceptions, however, if an exception
 * occurs it ends up calling std::terminate().
 *
 * \note
 * If a function throws, it logs information using the log facility.
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
        //
        log << log_level_t::fatal
            << "mutex::unlock() threw an exception while in the ~guard() function."
            << end;
        std::terminate();
    }
}


/** \brief Unlock the attached mutex or rwlock.
 *
 * This function can be called any number of times. If the one of the
 * mutex or rwlock object is currently locked, the function unlocks it,
 * otherwise nothing happens.
 *
 * If necessary, you can relock the mutex using the lock() function,
 * assuming you set the \p done parameter to false.
 *
 * This function may throw an exception if the unlock() call fails.
 *
 * \param[in] done  Whether you are done with this guard, if so, the
 * pointer will be set to null and you can then destroy the mutex or
 * rwlock (this is the default). If instead you want to be able to
 * re-lock the mutex or rwlock, then set this parameter to false.
 * The mutex or rwlock cannot be destroyed if this parameter is set
 * to false.
 *
 * \sa lock()
 */
void guard::unlock(bool done)
{
    if(f_locked)
    {
        mutex * m(f_mutex);
        recursive_rwlock * r(f_recursive_rwlock);
        rwlock * l(f_rwlock);
        f_locked = false;
        if(done)
        {
            f_mutex = nullptr;
            f_rwlock = nullptr;
        }
        if(m != nullptr)
        {
            m->unlock();
        }
        else if(l != nullptr)
        {
            l->unlock();
        }
        else if(r != nullptr)
        {
            r->unlock();
        }
    }
}


/** \brief Relock this mutex or rwlock.
 *
 * This function can be called any number of times. If called while the
 * mutex or rwlock is not locked (i.e. you called the unlock(false) function,
 * then it gets relocked, otherwise nothing happens.
 *
 * Note that when creating the guard, the mutex or rwlock are automatically
 * locked, so you rarely need to call this function.
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
 *             lock.unlock(false);
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
 * or rwlock are in a known state (i.e. unlocked when exiting the guarded
 * block).
 *
 * For a rwlock, the guard cannot changed from exclusive to non-exclusive
 * or vice versa. The exclusivity is defined in the constructor and it
 * cannot change later.
 *
 * \note
 * This implementation always attempts a mutex::lock() or rwlock::lock(),
 * checks whether it was necessary (i.e. is the f_locked flag false?) and
 * if not, it unlocks the mutex/rwlock (since the guard already had the
 * lock to itself).
 *
 * \warning
 * It is not 100% safe to call this function when the guard::unlock()
 * function is called with its done parameter set to true, which is the
 * default if you do not specify false in your call. It is safe if the
 * guard is only used on the stack.
 *
 * \warning
 * The rwlock in exclusive mode is not recursive. This function cannot
 * safely be called if you did not first call the unlock(false) function.
 * Doing so is likely to incorrectly unlock the rwlock or deadlock your
 * program.
 *
 * \sa unlock()
 */
void guard::lock()
{
    if(f_mutex != nullptr)
    {
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
    else if(f_recursive_rwlock != nullptr)
    {
        if(f_exclusive)
        {
            f_recursive_rwlock->lock();
        }
        else
        {
            f_recursive_rwlock->rlock();
        }
        if(f_locked)
        {
            f_recursive_rwlock->unlock();
        }
        else
        {
            f_locked = true;
        }
    }
    else if(f_rwlock != nullptr)
    {
        if(f_exclusive)
        {
            f_rwlock->lock();
        }
        else
        {
            f_rwlock->rlock();
        }
        if(f_locked)
        {
            f_rwlock->unlock();
        }
        else
        {
            f_locked = true;
        }
    }
}


/** \brief Check whether the lock this guard manages is current locked.
 *
 * This function returns the f_locked flag of the guard object. If true,
 * then the guard is expected to have its mutex or rwlock locked. If false,
 * then the guard mutex or rwlock is not currently locked.
 *
 * \warning
 * This function is not 100% safe if you call unlock() with its done
 * parameter set to true, which is the default. It is safe if the guard
 * is only used on the stack.
 *
 * \return true if the guard currently holds the lock.
 */
bool guard::is_locked() const
{
    bool result(false);
    if(f_mutex != nullptr)
    {
        f_mutex->lock();
        result = f_locked;
        f_mutex->unlock();
    }
    else if(f_recursive_rwlock != nullptr)
    {
        if(f_exclusive)
        {
            f_recursive_rwlock->lock();
            result = f_locked;
            f_recursive_rwlock->unlock();
        }
        else
        {
            f_recursive_rwlock->rlock();
            result = f_locked;
            f_recursive_rwlock->unlock();
        }
    }
    else if(f_rwlock != nullptr)
    {
        if(f_exclusive)
        {
            // a rwlock in exclusive mode is not recursive
            //
            return f_locked;
        }

        // TODO: a shared lock may not be exclusive if you created it
        //       with prefer_reader set to false
        //
        f_rwlock->rlock();
        result = f_locked;
        f_rwlock->unlock();
    }

    return result;
}




/** \fn guard::guard(guard const & rhs)
 * \brief The copy operator is deleted.
 *
 * The guard class saves a bare pointer to the mutex or rwlock it is
 * guarding. Because of that, a copy is not really possible and it's
 * also not useful.
 *
 * \param[in] rhs  The right hand side.
 */


/** \fn guard & guard::operator = (guard const & rhs)
 * \brief The assignment operator is deleted.
 *
 * The guard class saves a bare pointer to the mutex or rwlock it is
 * guarding. Because of that, an assignment is not really possible and
 * it's also not useful.
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
 * which is exception safe since it still gets unlocked when
 * an exception occurs.
 *
 * \warning
 * Note that it is not safe if you get a Unix signal. The lock
 * will very likely still be in place if such a signal happens
 * while within the lock.
 */


/** \var guard::f_rwlock
 * \brief The rwlock used by the guard class.
 *
 * Whenever you want to lock a part of your code so only one thread
 * runs it in read-write mode or any number of threads in read-only
 * mode, you want to use a guard. This guard makes use of a rwlock
 * that you pass to it on construction.
 *
 * The guard object keeps a reference to your rwlock and uses
 * it to lock on construction and unlock on destruction. This
 * generates a perfect safe guard around your code. Safe guard
 * which is exception safe since it still gets unlocked when
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
 * This flag is used to know whether the mutex or rwlock are currently
 * considered locked by the guard object.
 */


/** \var guard::f_exclusive
 * \brief Whether the rwlock guard is in exclusive mode.
 *
 * This flag is used to know whether the rwlock is considered to be exclusive
 * (read-write) or shared (read-only) mode.
 *
 * Note that this can only be set at construction time. Calling the lock()
 * function later does not give you the ability to change the mode.
 */



} // namespace cppthread
// vim: ts=4 sw=4 et
