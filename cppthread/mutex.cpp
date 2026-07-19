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
 * \brief Implementation of the mutex class.
 *
 * This file includes the implementation used by the mutex class.
 *
 * The mutex is used to lock areas that need to be executed by a single
 * thread at a time. This implementation allows for:
 *
 * * lock() -- wait until it can lock
 * * try_lock() -- try a lock, if it fails return immediately
 * * timed_wait() -- try to lock for the specified amount of time
 * * dated_wait() -- try to lock by the specified date
 * * signal() -- send a signal
 * * safe_signal() -- safely send a signal (slow than signal())
 * * wait() -- wait until a signal is received
 * * broadcast() -- send a signal to all the waiting threads
 */


// self
//
#include    "cppthread/mutex.h"

#include    "cppthread/exception.h"
#include    "cppthread/guard.h"
#include    "cppthread/log.h"


// snapdev
//
#include    <snapdev/timespec_ex.h>


// C
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



/** \brief The implementation of the mutex.
 *
 * We have an internal implementation of the mutex to avoid leaking the
 * pthread library to the outside.
 *
 * The class holds a mutex and a condition which we use to wait in various
 * circumstances such as when we pop items from a currently empty fifo.
 */
class mutex_impl
{
public:
    pthread_mutex_t     f_mutex = pthread_mutex_t();
    pthread_cond_t      f_condition = pthread_cond_t();
};


/** \var pthread_mutex_t mutex_impl::f_mutex
 * \brief Mutex to support guards & signals.
 *
 * This is the actual system mutex. It is useful to protect areas of code
 * that only one thread is allowed to access at a time.
 *
 * The mutex also supports a condition that it can wait on and signal. This
 * is useful to signal a new state such as the end of FIFO as any thread
 * waiting on that FIFO now needs to wake up a give up (no more messages
 * will be sent to the FIFO).
 */


/** \var pthread_mutex_t mutex_impl::f_condition
 * \brief Condition linked to the mutex to support signaling.
 *
 * We often have to signal that a thread is done in regard to something or
 * other. The condition wakes up another thread which can then take over the
 * next step of the work.
 *
 * The condition is used for that purpose as we can wait on it with the
 * attached mutex without wasting any CPU time.
 */



}



/** \class mutex
 * \brief A mutex object to ensures atomicity.
 *
 * This class is used by threads when some data accessed by more than
 * one thread is about to be accessed. In most cases it is used with the
 * guard class so it is safe even in the event an exception is raised.
 *
 * \code
 *     {
 *         cppthread::guard lock(f_mutex);
 *
 *         ...
 *     } // <-- auto-unlock here
 * \endcode
 *
 * The mutex also includes a condition variable which can be signaled
 * using the signal() function. This wakes threads that are currently
 * waiting on the condition with one of the wait() functions.
 *
 * \code
 *     {
 *         cppthread::guard lock(f_mutex);
 *
 *         if(<some condition>)
 *         {
 *             // this unlocks, waits, and relocks your mutex
 *             f_mutex.wait();
 *         }
 *
 *         ...
 *
 *         f_mutex.signal();      // signal one other thread
 *            // or
 *         f_mutex.broadcast();   // signal all the other threads
 *     } // <-- auto-unlock here
 * \endcode
 *
 * \note
 * We use a recursive mutex so you may lock the mutex any number of times.
 * It has to be unlocked that many times. If you use the guard class, then
 * the lock and unlock are fully automated and you won't run in any issues
 * other than potential dead-locks.
 *
 * \note
 * The mutex copy and assignment operators are deleted. We do not allow you
 * to copy a mutex because that is not a good idea. You probably have an
 * issue in your code if you think you need to make a copy of a mutex.
 * (although that our implementation, because of the `f_impl`, would allow for
 * copies, it's still not a good idea.)
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
 *    cppthread::guard lock(my_mutex)
 *    ... // protected code
 * }
 * \endcode
 *
 * The lock can be tried to see whether another thread already has the
 * lock and fail if so. See the try_lock() function.
 *
 * The class also includes a condition in order to send signals and wait
 * on signals. There are two ways to send signals and three ways to wait.
 * Note that to call any one of the wait functions you must first have the
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
 *          cppthread::guard lock(my_mutex);
 *          my_mutex.wait();
 *      }
 *
 *      // wait on the signal for the specified amount of time
 *      {
 *          cppthread::guard lock(my_mutex);
 *          my_mutex.timed_wait(1000000UL); // wait up to 1 second
 *      }
 *
 *      // wait on the signal for until date or later or the signal
 *      {
 *          cppthread::guard lock(my_mutex);
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
 * \exception invalid_error
 * If any one of the initialization functions fails, this exception is
 * raised. The function also logs the error.
 */
mutex::mutex()
    : f_impl(std::make_shared<detail::mutex_impl>())
{
    // initialize the mutex
    //
    pthread_mutexattr_t mattr;
    int err(pthread_mutexattr_init(&mattr));
    if(err != 0)
    {
        log << log_level_t::fatal
            << "a mutex attribute structure could not be initialized, error #"
            << err
            << " -- "
            << strerror(err)
            << end;
        throw invalid_error("pthread_muteattr_init() failed");
    }
    err = pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE);
    if(err != 0)
    {
        log << log_level_t::fatal
            << "a mutex attribute structure type could not be setup, error #"
            << err
            << " -- "
            << strerror(err)
            << end;
        pthread_mutexattr_destroy(&mattr);
        throw invalid_error("pthread_muteattr_settype() failed");
    }
    err = pthread_mutex_init(&f_impl->f_mutex, &mattr);
    if(err != 0)
    {
        log << log_level_t::fatal
            << "a mutex structure could not be initialized, error #"
            << err
            << " -- "
            << strerror(err)
            << end;
        pthread_mutexattr_destroy(&mattr);
        throw invalid_error("pthread_mutex_init() failed");
    }
    err = pthread_mutexattr_destroy(&mattr);
    if(err != 0)
    {
        log << log_level_t::fatal
            << "a mutex attribute structure could not be destroyed, error #"
            << err
            << " -- "
            << strerror(err)
            << end;
        pthread_mutex_destroy(&f_impl->f_mutex);
        throw invalid_error("pthread_mutexattr_destroy() failed");
    }

    // initialize the condition
    //
    pthread_condattr_t cattr;
    err = pthread_condattr_init(&cattr);
    if(err != 0)
    {
        log << log_level_t::fatal
            << "a mutex condition attribute structure could not be initialized, error #"
            << err
            << " -- "
            << strerror(err)
            << end;
        pthread_mutex_destroy(&f_impl->f_mutex);
        throw invalid_error("pthread_condattr_init() failed");
    }
    err = pthread_cond_init(&f_impl->f_condition, &cattr);
    if(err != 0)
    {
        log << log_level_t::fatal
            << "a mutex condition structure could not be initialized, error #"
            << err
            << " -- "
            << strerror(err)
            << end;
        pthread_condattr_destroy(&cattr);
        pthread_mutex_destroy(&f_impl->f_mutex);
        throw invalid_error("pthread_cond_init() failed");
    }
    err = pthread_condattr_destroy(&cattr);
    if(err != 0)
    {
        log << log_level_t::fatal
            << "a mutex condition attribute structure could not be destroyed, error #"
            << err
            << " -- "
            << strerror(err)
            << end;
        pthread_mutex_destroy(&f_impl->f_mutex);
        throw invalid_error("pthread_condattr_destroy() failed");
    }
}


/** \brief Clean up a mutex object.
 *
 * This function ensures that the mutex object is cleaned up, which means
 * the mutex and conditions get destroyed.
 *
 * This destructor verifies that the mutex is not currently locked. A
 * locked mutex cannot be destroyed. If still locked, then an error is
 * sent to the logger and the function calls std::terminate().
 */
mutex::~mutex()
{
    if(f_reference_count != 0UL)
    {
        // we cannot legally throw in a destructor so we instead generate a fatal error
        // WARNING: the log << ... may generate a deadlock
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
            << " -- "
            << strerror(err)
            << end;
    }
    err = pthread_mutex_destroy(&f_impl->f_mutex);
    if(err != 0)
    {
        log << log_level_t::fatal
            << "a mutex destruction generated error #"
            << err
            << " -- "
            << strerror(err)
            << end;
    }
}


/** \brief Lock a mutex
 *
 * This function locks the mutex. The function waits until the mutex is
 * available if it is not currently available. To avoid waiting one may
 * want to use the try_lock() function instead.
 *
 * \exception invalid_error
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
        throw invalid_error("pthread_mutex_lock() failed");
    }
    ++f_reference_count;
}


/** \brief Try locking the mutex.
 *
 * This function tries locking the mutex. If the mutex cannot be locked
 * because another process already locked it, then the function returns
 * immediately with false.
 *
 * \exception invalid_error
 * If the lock fails, this exception is raised.
 *
 * \return true if the lock succeeded, false otherwise.
 */
bool mutex::try_lock()
{
    int const err(pthread_mutex_trylock(&f_impl->f_mutex));
    if(err == 0)
    {
        ++f_reference_count;
        return true;
    }

    // failed because another thread has the lock?
    //
    if(err == EBUSY)
    {
        return false;
    }

    // another type of failure
    //
    log << log_level_t::error
        << "a mutex try lock generated error #"
        << err
        << " -- "
        << strerror(err)
        << end;
    throw invalid_error("pthread_mutex_trylock() failed");
}


/** \brief Unlock a mutex.
 *
 * This function unlock the specified mutex. The function must be called
 * exactly once per call to the lock() function, or successful call to
 * the try_lock() function.
 *
 * The unlock never waits.
 *
 * It is safer to make use of the cppthread::guard to ensure the same
 * number of lock() and unlock() to a mutex.
 *
 * \exception invalid_error
 * If the unlock fails, this exception is raised.
 *
 * \exception not_locked
 * If the function is called too many times, then the lock count is going
 * to be zero and this exception will be raised.
 */
void mutex::unlock()
{
    // we can't unlock if it wasn't locked before!
    //
    if(f_reference_count == 0UL)
    {
        log << log_level_t::fatal
            << "attempting to unlock a mutex when it is not currently locked."
            << end;
        throw not_locked("unlock was called too many times");
    }
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
        throw invalid_error("pthread_mutex_unlock() failed");
    }
}


/** \brief Verify that the mutex is locked exactly once.
 *
 * This function makes sure that the mutex is locked exactly once,
 * which is important when ready to wait() on a signal.
 *
 * For any wait on a mutex condition to work, it \b must have the mutex
 * locked exactly once. It cannot be unlocked nor locked more than once.
 * According to the documentation, a recursive lock is "dangerous"
 * when used with a condition variable because the kernel only unlock()
 * the mutex once before performing the wait. Since we have a reference
 * counter, we can verify that this is the case and fail in case it is
 * not.
 *
 * \note
 * The wait functions are expected to be called within a guarded block.
 *
 * \note
 * Just in case, I created a test (see tests/recursive_mutex_test.cpp)
 * which verifies that this is indeed true and it is. The pthread
 * implementation does not verify anything and if the unlock does
 * not properly give other threads the chance to send signals or
 * the lock was not locked at all, you get some weird behaviors
 * or just a plain old deadlock.
 *
 * \exception not_locked_once_error
 * The mutex is not locked exactly once. The wait() won't work properly
 * and would likely generate a deadlock.
 */
void mutex::is_locked_once()
{
    // to make this test a strong test, we need to guard the function
    //
    // i.e. if the current thread does not have a lock at all, then
    //      this guard waits on other threads having such; then it
    //      locks once and we fail the following test which expects
    //      two locks to be in place (the lock of the wait() caller
    //      and this very lock)
    //
    guard lock(*this);

    if(f_reference_count != 2UL)
    {
        std::uint32_t const reference_count(f_reference_count - 1UL);
        lock.unlock();

        // if reference_count != 0 then the log << ... may generate a
        // deadlock; I'm not too sure how to avoid that one at the moment
        //
        //log << log_level_t::fatal
        //    << "attempt to wait on a mutex when it is not locked exactly once; current count is "
        //    << reference_count
        //    << '.'
        //    << end;
        throw not_locked_once("a wait function requires the mutex to be locked exactly once.");
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
 * This function cannot be called if the mutex is not locked exactly once.
 *
 * \exception mutex_failed
 * This exception is raised in the event the conditional wait fails.
 */
void mutex::wait()
{
    is_locked_once();

    --f_reference_count;
    int const err(pthread_cond_wait(&f_impl->f_condition, &f_impl->f_mutex));
    ++f_reference_count;
    if(err != 0)
    {
        // an error occurred!
        //
        // Here the mutex should be locked so a log << ... could deadlock
        // since this very mutex is expected to be locked at this point
        //
        //log << log_level_t::fatal
        //    << "a mutex conditional wait generated error #"
        //    << err
        //    << " -- "
        //    << strerror(err)
        //    << end;
        throw mutex_failed("pthread_cond_wait() failed");
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
 * This function cannot be called if the mutex is not locked exactly once.
 *
 * \exception mutex_failed
 * This exception is raised when the mutex wait function fails.
 *
 * \param[in] usecs  The maximum number of micro seconds to wait until you
 *                   receive the signal.
 *
 * \return true if the condition was raised, false if the wait timed out.
 */
bool mutex::timed_wait(std::uint64_t const usecs)
{
    return timed_wait(timespec{
              static_cast<time_t>(usecs / 1'000'000ULL)
            , static_cast<long>((usecs % 1'000'000ULL) * 1'000ULL)
        });
}


/** \brief Wait on a mutex condition with a time limit.
 *
 * At times it is useful to wait on a mutex to become available without
 * polling the mutex, but only for some time. This function waits for
 * the number of specified nano seconds. The function returns early if
 * the condition was triggered. Otherwise it waits until the specified
 * number of nano seconds elapsed and then returns.
 *
 * \warning
 * This function cannot be called if the mutex is not locked exactly once.
 *
 * \exception mutex_failed
 * This exception is raised when the mutex wait function fails.
 *
 * \param[in] nsecs  The maximum number of nano seconds to wait until you
 *                   receive the signal.
 *
 * \return true if the condition was raised, false if the wait timed out.
 *
 * \sa dated_wait(timespec date)
 */
bool mutex::timed_wait(timespec const & nsecs)
{
    is_locked_once();

    // get clock time (a.k.a. now)
    //
    snapdev::timespec_ex abstime(snapdev::timespec_ex::gettime());

    // now + user specified nsecs
    //
    abstime += nsecs;

    --f_reference_count;
    int const err(pthread_cond_timedwait(
              &f_impl->f_condition
            , &f_impl->f_mutex
            , &abstime));
    ++f_reference_count;
    if(err != 0)
    {
        if(err == ETIMEDOUT)
        {
            return false;
        }

        // an error occurred!
        //
        // Here the mutex should be locked so a log << ... could deadlock
        // since this very mutex is expected to be locked at this point
        //
        //log << log_level_t::fatal
        //    << "a mutex conditional timed wait generated error #"
        //    << err
        //    << " -- "
        //    << strerror(err)
        //    << " (time out sec = "
        //    << abstime.tv_sec
        //    << ", nsec = "
        //    << abstime.tv_nsec
        //    << ")"
        //    << end;
        throw mutex_failed("pthread_cond_timedwait() failed");
    }

    return true;
}


/** \brief Wait on a mutex until the specified date.
 *
 * This function waits on the mutex condition to be signaled up until the
 * specified date is passed.
 *
 * \warning
 * This function cannot be called if the mutex is not locked exactly once.
 *
 * \exception mutex_failed
 * This exception is raised whenever the thread wait function fails.
 *
 * \param[in] usec  The date when the mutex times out in microseconds.
 *
 * \return true if the condition occurs before the function times out,
 *         false if the function times out.
 */
bool mutex::dated_wait(std::uint64_t const usec)
{
    return dated_wait(timespec{
              static_cast<long>(usec / 1'000'000ULL)
            , static_cast<long>((usec % 1'000'000ULL) * 1'000ULL)
        });
}


/** \brief Wait on a mutex until the specified date.
 *
 * This function waits on the mutex condition to be signaled up until the
 * specified date is passed.
 *
 * \warning
 * This function cannot be called if the mutex is not locked exactly once.
 *
 * \exception mutex_failed
 * This exception is raised whenever the thread wait function fails.
 *
 * \param[in] date  The date when the mutex times out in nanoseconds.
 *
 * \return true if the condition occurs before the function times out,
 *         false if the function times out.
 */
bool mutex::dated_wait(timespec const & date)
{
    is_locked_once();

    --f_reference_count;
    int const err(pthread_cond_timedwait(
              &f_impl->f_condition
            , &f_impl->f_mutex
            , &date));
    ++f_reference_count;
    if(err != 0)
    {
        if(err == ETIMEDOUT)
        {
            return false;
        }

        // an error occurred!
        //
        // Here the mutex should be locked so a log << ... could deadlock
        // since this very mutex is expected to be locked at this point
        //
        //log << log_level_t::error
        //    << "a mutex conditional wait generated error #"
        //    << err
        //    << " -- "
        //    << strerror(err)
        //    << " (time out sec = "
        //    << date.tv_sec
        //    << ", nsec = "
        //    << date.tv_nsec
        //    << ")"
        //    << end;
        throw mutex_failed("pthread_cond_timedwait() failed");
    }

    return true;
}


/** \brief Signal at least one mutex.
 *
 * Our mutexes include a condition that get signaled by calling this
 * function. This function wakes up one or more listening threads.
 *
 * The function does not lock the mutext before sending the signal. This
 * is useful if you already are in a guarded block or you do not mind
 * waking up more than one thread as a result of the call.
 *
 * \note
 * If you need to wake up exactly one other thread, then make sure to
 * use the safe_signal() function instead.
 *
 * \exception invalid_error
 * If one of the pthread system functions return an error, the function
 * raises this exception.
 *
 * \sa safe_signal()
 */
void mutex::signal()
{
    int const err(pthread_cond_signal(&f_impl->f_condition));
    if(err != 0)
    {
        log << log_level_t::fatal
            << "a mutex condition signal generated error #"
            << err
            << " -- "
            << strerror(err)
            << end;
        throw invalid_error("pthread_cond_signal() failed");
    }
}


/** \brief Signal a mutex.
 *
 * Our mutexes include a condition that get signaled by calling this
 * function. This function wakes up one listening thread.
 *
 * The function ensures that the mutex is locked before sending
 * the signal so you do not have to lock the mutex yourself.
 *
 * \exception invalid_error
 * If one of the pthread system functions return an error, the function
 * raises this exception.
 *
 * \sa signal()
 */
void mutex::safe_signal()
{
    guard lock(*this);

    int const err(pthread_cond_signal(&f_impl->f_condition));
    lock.unlock();
    if(err != 0)
    {
        log << log_level_t::fatal
            << "a mutex condition signal generated error #"
            << err
            << " -- "
            << strerror(err)
            << end;
        throw invalid_error("pthread_cond_signal() failed");
    }
}


/** \brief Broadcast a mutex signal.
 *
 * Our mutexes include a condition that get signaled by calling this
 * function. This function actually signals all the threads that are
 * currently waiting for the mutex signal. The order in which the
 * threads get awaken is unspecified.
 *
 * The function ensures that the mutex is locked before broadcasting
 * the signal so you do not have to lock the mutex yourself.
 *
 * \note
 * We also offer a safe_broadcast(). If you expect absolutely all the
 * other threads to receive the signal, then make sure to use the
 * safe_broadcast() function instead. However, if you are already in
 * a guarded block, then there is no need for an additional lock and
 * this function will work exactly as expected.
 *
 * \exception invalid_error
 * If one of the pthread system functions return an error, the function
 * raises this exception.
 *
 * \sa safe_broadcast()
 */
void mutex::broadcast()
{
    int const err(pthread_cond_broadcast(&f_impl->f_condition));
    if(err != 0)
    {
        log << log_level_t::fatal
            << "a mutex signal broadcast generated error #"
            << err
            << " -- "
            << strerror(err)
            << end;
        throw invalid_error("pthread_cond_broadcast() failed");
    }
}


/** \brief Broadcast a mutex signal.
 *
 * Our mutexes include a condition that get signaled by calling this
 * function. This function actually signals all the threads that are
 * currently waiting for the mutex signal. The order in which the
 * threads get awaken is unspecified.
 *
 * The function ensures that the mutex is locked before broadcasting
 * the signal so you do not have to lock the mutex yourself. Note that
 * is not required to lock a mutex before broadcasting a signal. The
 * effects are similar.
 *
 * \exception invalid_error
 * If one of the pthread system functions return an error, the function
 * raises this exception.
 *
 * \sa broadcast()
 */
void mutex::safe_broadcast()
{
    guard lock(*this);

    int const err(pthread_cond_broadcast(&f_impl->f_condition));
    lock.unlock();
    if(err != 0)
    {
        log << log_level_t::fatal
            << "a mutex signal broadcast generated error #"
            << err
            << " -- "
            << strerror(err)
            << end;
        throw invalid_error("pthread_cond_broadcast() failed");
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


/** \brief This is an internal function called by the logger constructor.
 *
 * The logger constructor is called whenever the cppthread library is loaded
 * and that happens in your main thread at initialization time. This means
 * any of your code can access the g_system_mutex as required.
 *
 * This function is not defined externally so that other users can't call
 * it from the outside (there would be no need anyway). Just in case, if
 * called a second time, the function writes an error message and fails
 * with a call to std::terminate().
 *
 * Another way to safely create a new mutex is to use a static variable
 * in a function. The initialization of that static variable is always
 * safe so that mutex is available to all your threads even if you already
 * had multiple threads the first time that function was called. This safety
 * is guaranteed by C++ which uses its own mutex to initialize static
 * variables.
 *
 * \code
 *     void myfunc()
 *     {
 *         static cppthread::mutex m = cppthread::mutex();
 *
 *         // here "m" is a safe to use mutex
 *         cppthread::guard lock(m);
 *
 *         ...do atomic work here...
 *     }
 * \endcode
 *
 * You could even create a function which returns a reference to that
 * local mutex (just make sure you don't copy that mutex).
 *
 * \note
 * Although this function gets called from the logger constructor, it is not
 * used by the logger at all.
 */
void create_system_mutex()
{
    if(g_system_mutex != nullptr)
    {
        std::cerr << "fatal: create_system_mutex() called twice." << std::endl;
        std::terminate();
    }

    g_system_mutex = new mutex;
}




/** \typedef mutex::pointer_t
 * \brief Shared pointer to a mutex.
 *
 * You can allocate mutexes as global variables, local variables, variable
 * members and once in a while you  may want to allocate them. In that
 * case, we suggest that you use a shared pointer.
 */


/** \typedef mutex::vector_t
 * \brief A vector of mutexes.
 *
 * This type defines a vector that can be used to manage mutexes. In
 * this case, the mutexes must be allocated.
 */


/** \typedef mutex::direct_vector_t
 * \brief A vector of mutexes.
 *
 * This type defines a vector of direct mutexes (i.e. not pointers to
 * mutexes). This type can be used when you know the number of mutexes
 * to allocate. Dynamically adding/removing mutexes with this type
 * can be rather complicated.
 */


/** \var mutex::f_impl
 * \brief The pthread mutex implementation.
 *
 * This variable member holds the actual pthread mutex. The mutex
 * implementation manages this field as required.
 */


/** \var mutex::f_reference_count
 * \brief The lock reference count.
 *
 * The mutex tracks the number of times the mutex gets locked.
 * In the end, the lock reference must be zero for the mutex
 * to be properly destroyed.
 */




} // namespace cppthread
// vim: ts=4 sw=4 et
