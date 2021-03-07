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

/** \file
 * \brief Implementation of the Thread Runner and Managers.
 *
 * This file includes the implementation used by the cppthread environment.
 */


// self
//
#include    "cppthread/mutex.h"

#include    "cppthread/exception.h"
#include    "cppthread/guard.h"
#include    "cppthread/log.h"


// C lib
//
#include    <string.h>
#include    <sys/time.h>


// last include
//
#include    <snapdev/poison.h>




namespace cppthread
{



namespace detail
{



class mutex_impl
{
public:
    pthread_mutex_t     f_mutex = pthread_mutex_t();
    pthread_cond_t      f_condition = pthread_cond_t();
};



}



/** \class mutex
 * \brief A mutex object to ensures atomicity.
 *
 * This class is used by threads when some data accessed by more than
 * one thread is about to be accessed. In most cases it is used with the
 * guard class so it is safe even in the event an exception is raised.
 *
 * The mutex also includes a condition variable which can be signaled
 * using the signal() function. This wakes threads that are currently
 * waiting on the condition with one of the wait() functions.
 *
 * \note
 * We use a recursive mutex so you may lock the mutex any number of times.
 * It has to be unlocked that many times, of course.
 */

/** \var mutex::f_mutex
 * \brief The pthread mutex.
 *
 * This variable member holds the pthread mutex. The mutex
 * implementation manages this field as required.
 */


/** \brief An inter-thread mutex to ensure unicity of execution.
 *
 * The mutex object is used to lock part of the code that needs to be run
 * by only one thread at a time. This is also called a critical section
 * and a memory barrier.
 *
 * In most cases one uses the guard object to temporarily lock
 * the mutex using the FIFO to help ensure the mutex gets unlocked as
 * required in the event an exception occurs.
 *
 * \code
 * {
 *    cppthread::guard lock(&my_mutex)
 *    ... // protected code
 * }
 * \endcode
 *
 * The lock can be tried to see whether another thread already has the
 * lock and fail if so. See the try_lock() function.
 *
 * The class also includes a condition in order to send signals and wait
 * on signals. There are two ways to send signals and three ways to wait.
 * Note that to call any one of the wait funtions you must first have the
 * mutex locked, what otherwise happens is undefined.
 *
 * \code
 * {
 *      // wake one waiting thread
 *      my_mutex.signal();
 *
 *      // wake all the waiting thread
 *      my_mutex.broadcast();
 *
 *      // wait on the signal forever
 *      {
 *          cppthread::guard lock(&my_mutex);
 *          my_mutex.wait();
 *      }
 *
 *      // wait on the signal for the specified amount of time
 *      {
 *          cppthread::guard lock(&my_mutex);
 *          my_mutex.timed_wait(1000000UL); // wait up to 1 second
 *      }
 *
 *      // wait on the signal for until date or later or the signal
 *      {
 *          cppthread::guard lock(&my_mutex);
 *          my_mutex.dated_wait(date); // wait on signal or until date
 *      }
 * }
 * \endcode
 *
 * If you need a FIFO of messages between your threads, look at the
 * snap_fifo template.
 *
 * \note
 * Care must be used to always initialized a mutex before it
 * is possibly accessed by more than one thread. This is usually
 * the case in the constructor of your objects.
 *
 * \exception cppthread_exception_invalid_error
 * If any one of the initialization functions fails, this exception is
 * raised. The function also logs the error.
 */
mutex::mutex()
    : f_impl(std::make_shared<detail::mutex_impl>())
{
    // initialize the mutex
    pthread_mutexattr_t mattr;
    int err(pthread_mutexattr_init(&mattr));
    if(err != 0)
    {
        log << log_level_t::fatal
            << "a mutex attribute structure could not be initialized, error #"
            << err
            << end;
        throw cppthread_invalid_error("pthread_muteattr_init() failed");
    }
    err = pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE);
    if(err != 0)
    {
        log << log_level_t::fatal
            << "a mutex attribute structure type could not be setup, error #"
            << err
            << end;
        pthread_mutexattr_destroy(&mattr);
        throw cppthread_invalid_error("pthread_muteattr_settype() failed");
    }
    err = pthread_mutex_init(&f_impl->f_mutex, &mattr);
    if(err != 0)
    {
        log << log_level_t::fatal
            << "a mutex structure could not be initialized, error #"
            << err
            << end;
        pthread_mutexattr_destroy(&mattr);
        throw cppthread_invalid_error("pthread_mutex_init() failed");
    }
    err = pthread_mutexattr_destroy(&mattr);
    if(err != 0)
    {
        log << log_level_t::fatal
            << "a mutex attribute structure could not be destroyed, error #"
            << err
            << end;
        pthread_mutex_destroy(&f_impl->f_mutex);
        throw cppthread_invalid_error("pthread_mutexattr_destroy() failed");
    }

    // initialize the condition
    pthread_condattr_t cattr;
    err = pthread_condattr_init(&cattr);
    if(err != 0)
    {
        log << log_level_t::fatal
            << "a mutex condition attribute structure could not be initialized, error #"
            << err
            << end;
        pthread_mutex_destroy(&f_impl->f_mutex);
        throw cppthread_invalid_error("pthread_condattr_init() failed");
    }
    err = pthread_cond_init(&f_impl->f_condition, &cattr);
    if(err != 0)
    {
        log << log_level_t::fatal
            << "a mutex condition structure could not be initialized, error #"
            << err
            << end;
        pthread_condattr_destroy(&cattr);
        pthread_mutex_destroy(&f_impl->f_mutex);
        throw cppthread_invalid_error("pthread_cond_init() failed");
    }
    err = pthread_condattr_destroy(&cattr);
    if(err != 0)
    {
        log << log_level_t::fatal
            << "a mutex condition attribute structure could not be destroyed, error #"
            << err
            << end;
        pthread_mutex_destroy(&f_impl->f_mutex);
        throw cppthread_invalid_error("pthread_condattr_destroy() failed");
    }
}


/** \brief Clean up a mutex object.
 *
 * This function ensures that the mutex object is cleaned up, which means
 * the mutex and conditions get destroyed.
 *
 * This destructor verifies that the mutex is not currently locked. A
 * locked mutex can't be destroyed. If still locked, then an error is
 * sent to the logger and the function calls exit(1).
 */
mutex::~mutex()
{
    // Note that the following reference count test only ensure that
    // you don't delete a mutex which is still locked; however, if
    // you still have multiple threads running, we can't really know
    // if another thread is not just about to use this thread...
    //
    if(f_reference_count != 0UL)
    {
        // we cannot legally throw in a destructor so we instead generate a fatal error
        //
        log << log_level_t::fatal
            << "a mutex is being destroyed when its reference count is "
            << f_reference_count
            << " instead of zero."
            << end;
        std::terminate();
    }
    int err(pthread_cond_destroy(&f_impl->f_condition));
    if(err != 0)
    {
        log << log_level_t::error
            << "a mutex condition destruction generated error #"
            << err
            << end;
    }
    err = pthread_mutex_destroy(&f_impl->f_mutex);
    if(err != 0)
    {
        log << log_level_t::fatal
            << "a mutex destruction generated error #"
            << err
            << end;
    }
}


/** \brief Lock a mutex
 *
 * This function locks the mutex. The function waits until the mutex is
 * available if it is not currently available. To avoid waiting one may
 * want to use the try_lock() function instead.
 *
 * Although the function cannot fail, the call can lock up a process if
 * two or more mutexes are used and another thread is already waiting
 * on this process.
 *
 * \exception cppthread_exception_invalid_error
 * If the lock fails, this exception is raised.
 */
void mutex::lock()
{
    int const err(pthread_mutex_lock(&f_impl->f_mutex));
    if(err != 0)
    {
        log << log_level_t::error
            << "a mutex lock generated error #"
            << err
            << " -- "
            << strerror(err)
            << end;
        throw cppthread_invalid_error("pthread_mutex_lock() failed");
    }

    // note: we do not need an atomic call since we
    //       already know we are running alone here...
    ++f_reference_count;
}


/** \brief Try locking the mutex.
 *
 * This function tries locking the mutex. If the mutex cannot be locked
 * because another process already locked it, then the function returns
 * immediately with false.
 *
 * \exception cppthread_exception_invalid_error
 * If the lock fails, this exception is raised.
 *
 * \return true if the lock succeeded, false otherwise.
 */
bool mutex::try_lock()
{
    int const err(pthread_mutex_trylock(&f_impl->f_mutex));
    if(err == 0)
    {
        // note: we do not need an atomic call since we
        //       already know we are running alone here...
        ++f_reference_count;
        return true;
    }

    // failed because another thread has the lock?
    if(err == EBUSY)
    {
        return false;
    }

    // another type of failure
    log << log_level_t::error
        << "a mutex try lock generated error #"
        << err
        << " -- "
        << strerror(err)
        << end;
    throw cppthread_invalid_error("pthread_mutex_trylock() failed");
}


/** \brief Unlock a mutex.
 *
 * This function unlock the specified mutex. The function must be called
 * exactly once per call to the lock() function, or successful call to
 * the try_lock() function.
 *
 * The unlock never waits.
 *
 * \exception cppthread_exception_invalid_error
 * If the unlock fails, this exception is raised.
 *
 * \exception cppthread_exception_not_locked_error
 * If the function is called too many times, then the lock count is going
 * to be zero and this exception will be raised.
 */
void mutex::unlock()
{
    // We can't unlock if it wasn't locked before!
    if(f_reference_count <= 0UL)
    {
        log << log_level_t::fatal
            << "attempting to unlock a mutex when it is still locked "
            << f_reference_count
            << " times"
            << end;
        throw cppthread_not_locked_error("unlock was called too many times");
    }

    // NOTE: we do not need an atomic call since we
    //       already know we are running alone here...
    --f_reference_count;

    int const err(pthread_mutex_unlock(&f_impl->f_mutex));
    if(err != 0)
    {
        log << log_level_t::fatal
            << "a mutex unlock generated error #"
            << err
            << " -- "
            << strerror(err)
            << end;
        throw cppthread_invalid_error("pthread_mutex_unlock() failed");
    }
}


/** \brief Wait on a mutex condition.
 *
 * At times it is useful to wait on a mutex to become available without
 * polling the mutex (which uselessly wastes precious processing time.)
 * This function can be used to wait on a mutex condition.
 *
 * This version of the wait() blocks until a signal is received.
 *
 * \warning
 * This function cannot be called if the mutex is not locked or the
 * wait will fail in unpredicatable ways.
 *
 * \exception cppthread_exception_not_locked_once_error
 * This exception is raised if the reference count is not exactly 1.
 * In other words, the mutex must be locked by the caller but only
 * one time.
 *
 * \exception cppthread_exception_mutex_failed_error
 * This exception is raised in the event the conditional wait fails.
 */
void mutex::wait()
{
    // For any mutex wait to work, we MUST have the
    // mutex locked already and just one time.
    //
    // note: the 1 time is just for assurance that it will
    //       work in most cases; it should work even when locked
    //       multiple times, but it is less likely. For sure, it
    //       has to be at least once.
    //if(f_reference_count != 1UL)
    //{
    //    log << log_level_t::fatal
    //        << "attempting to wait on a mutex when it is not locked exactly once, current count is "
    //        << f_reference_count
    //        << end;
    //    throw cppthread_exception_not_locked_once_error();
    //}
    int const err(pthread_cond_wait(&f_impl->f_condition, &f_impl->f_mutex));
    if(err != 0)
    {
        // an error occurred!
        log << log_level_t::fatal
            << "a mutex conditional wait generated error #"
            << err
            << " -- "
            << strerror(err)
            << end;
        throw cppthread_mutex_failed_error("pthread_cond_wait() failed");
    }
}


/** \brief Wait on a mutex condition with a time limit.
 *
 * At times it is useful to wait on a mutex to become available without
 * polling the mutex, but only for some time. This function waits for
 * the number of specified micro seconds. The function returns early if
 * the condition was triggered. Otherwise it waits until the specified
 * number of micro seconds elapsed and then returns.
 *
 * \warning
 * This function cannot be called if the mutex is not locked or the
 * wait will fail in unpredicatable ways.
 *
 * \exception cppthread_exception_system_error
 * This exception is raised if a function returns an unexpected error.
 *
 * \exception cppthread_exception_mutex_failed_error
 * This exception is raised when the mutex wait function fails.
 *
 * \param[in] usecs  The maximum number of micro seconds to wait until you
 *                   receive the signal.
 *
 * \return true if the condition was raised, false if the wait timed out.
 */
bool mutex::timed_wait(uint64_t const usecs)
{
    // For any mutex wait to work, we MUST have the
    // mutex locked already and just one time.
    //
    // note: the 1 time is just for assurance that it will
    //       work in most cases; it should work even when locked
    //       multiple times, but it is less likely. For sure, it
    //       has to be at least once.
    //if(f_reference_count != 1UL)
    //{
    //  log << log_level_t::fatal
    //         << "attempting to timed wait "
    //         << usec
    //         << " usec on a mutex when it is not locked exactly once, current count is "
    //         << f_reference_count
    //         << end;
    //    throw cppthread_exception_not_locked_once_error();
    //}

    int err(0);

    // get time now
    struct timespec abstime;
    if(clock_gettime(CLOCK_REALTIME, &abstime) != 0)
    {
        err = errno;
        log << log_level_t::fatal
            << "gettimeofday() failed with errno: "
            << err
            << " -- "
            << strerror(err)
            << end;
        throw cppthread_system_error("gettimeofday() failed");
    }

    // now + user specified usec
    abstime.tv_sec += usecs / 1'000'000ULL;
    std::uint64_t nanos(abstime.tv_nsec + (usecs % 1'000'000ULL) * 1'000ULL);
    if(nanos > 1'000'000'000ULL)
    {
        ++abstime.tv_sec;
        nanos -= 1'000'000'000ULL;
    }
    abstime.tv_nsec = static_cast<long>(nanos);

    err = pthread_cond_timedwait(&f_impl->f_condition, &f_impl->f_mutex, &abstime);
    if(err != 0)
    {
        if(err == ETIMEDOUT)
        {
            return false;
        }

        // an error occurred!
        log << log_level_t::fatal
            << "a mutex conditional timed wait generated error #"
            << err
            << " -- "
            << strerror(err)
            << " (time out sec = "
            << abstime.tv_sec
            << ", nsec = "
            << abstime.tv_nsec
            << ")"
            << end;
        throw cppthread_mutex_failed_error("pthread_cond_timedwait() failed");
    }

    return true;
}


/** \brief Wait on a mutex until the specified date.
 *
 * This function waits on the mutex condition to be signaled up until the
 * specified date is passed.
 *
 * \warning
 * This function cannot be called if the mutex is not locked or the
 * wait will fail in unpredicatable ways.
 *
 * \exception cppthread_exception_mutex_failed_error
 * This exception is raised whenever the thread wait functionf fails.
 *
 * \param[in] usec  The date when the mutex times out in microseconds.
 *
 * \return true if the condition occurs before the function times out,
 *         false if the function times out.
 */
bool mutex::dated_wait(uint64_t usec)
{
    // For any mutex wait to work, we MUST have the
    // mutex locked already and just one time.
    //
    // note: the 1 time is just for assurance that it will
    //       work in most cases; it should work even when locked
    //       multiple times, but it is less likely. For sure, it
    //       has to be at least once.
    //if(f_reference_count != 1UL)
    //{
    //    log << log_level_t::fatal
    //        << "attempting to dated wait until "
    //        << usec
    //        << " msec on a mutex when it is not locked exactly once, current count is "
    //        << f_reference_count
    //        << end;
    //    throw cppthread_exception_not_locked_once_error();
    //}

    // setup the timeout date
    struct timespec timeout;
    timeout.tv_sec = static_cast<long>(usec / 1000000ULL);
    timeout.tv_nsec = static_cast<long>((usec % 1000000ULL) * 1000ULL);

    int const err(pthread_cond_timedwait(&f_impl->f_condition, &f_impl->f_mutex, &timeout));
    if(err != 0)
    {
        if(err == ETIMEDOUT)
        {
            return false;
        }

        // an error occurred!
        log << log_level_t::error
            << "a mutex conditional dated wait generated error #"
            << err
            << " -- "
            << strerror(err)
            << end;
        throw cppthread_mutex_failed_error("pthread_cond_timedwait() failed");
    }

    return true;
}


/** \brief Signal a mutex.
 *
 * Our mutexes include a condition that get signaled by calling this
 * function. This function wakes up one listening thread.
 *
 * The function ensures that the mutex is locked before broadcasting
 * the signal so you do not have to lock the mutex yourself.
 *
 * \exception cppthread_exception_invalid_error
 * If one of the pthread system functions return an error, the function
 * raises this exception.
 */
void mutex::signal()
{
    guard lock(*this);

    int const err(pthread_cond_signal(&f_impl->f_condition));
    if(err != 0)
    {
        log << log_level_t::fatal
            << "a mutex condition signal generated error #"
            << err
            << end;
        throw cppthread_invalid_error("pthread_cond_signal() failed");
    }
}


/** \brief Broadcast a mutex signal.
 *
 * Our mutexes include a condition that get signaled by calling this
 * function. This function actually signals all the threads that are
 * currently listening to the mutex signal. The order in which the
 * threads get awaken is unspecified.
 *
 * The function ensures that the mutex is locked before broadcasting
 * the signal so you do not have to lock the mutex yourself.
 *
 * \exception cppthread_exception_invalid_error
 * If one of the pthread system functions return an error, the function
 * raises this exception.
 */
void mutex::broadcast()
{
    guard lock(*this);

    int const err(pthread_cond_broadcast(&f_impl->f_condition));
    if(err != 0)
    {
        log << log_level_t::fatal
            << "a mutex signal broadcast generated error #"
            << err
            << end;
        throw cppthread_invalid_error("pthread_cond_broadcast() failed");
    }
}


/** \brief The system mutex.
 *
 * This mutex is created and initialized whenever this library is
 * loaded. This means it will always be ready for any library
 * that depends on the cppthread library.
 *
 * It should be used for things that may happen at any time and
 * require to be done by one thread at a time. For example, a
 * class with a get_instance() may want to lock this mutex, do
 * it's instance creation and then unlock the mutex.
 *
 * The mutex locking should be done using the guard class.
 *
 * \code
 *     ptr * get_instance()
 *     {
 *         cppthread::guard lock(*g_system_mutex);
 *
 *         ...proceed with your instance allocation...
 *     }
 * \endcode
 *
 * Because of the order in which things get initialized under
 * Linux, you can be sure that this mutex will be ready for
 * use as soon as the cppthread is loaded. So you can even
 * use it in your own C++ initialization (i.e. your global
 * objects.)
 *
 * Obviously, if you also have your own mutex, you need to
 * initialize it and therefore you may have problems in
 * your initialization process (i.e. your C++ globals may
 * not get initialized in the correct order--the mutex may
 * get initialized after your other parameters...)
 */
mutex *         g_system_mutex = nullptr;

void create_system_mutex()
{
    g_system_mutex = new mutex;
}



} // namespace cppthread
// vim: ts=4 sw=4 et
