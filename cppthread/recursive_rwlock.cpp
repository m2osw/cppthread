// Copyright (c) 2026  Made to Order Software Corp.  All Rights Reserved
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
 * \brief Implementation of the recursive_rwlock class.
 *
 * This file includes the implementation used by the recursive_rwlock class.
 *
 * The recursive_rwlock is used to lock areas that need to be executed by
 * a single thread at a time (write lock or exclusive mode) or by any
 * number of threads that can concurrently use a resource in read-only
 * mode (read lock or shared mode).
 *
 * This version uses the basic and fast pthread read-write lock plus a
 * mutex to manage the recursivity. This means the class is not
 * exceptionally fast because it needs to track all the shared locks
 * using an std::map. There is an additional mutex involved too.
 *
 * This implementation allows for:
 *
 * * lock() -- wait until it can lock exclusively
 * * rlock() -- wait until it can lock concurrently (read-only mode)
 * * try_lock() -- try an exclusive lock, if it fails return immediately
 * * try_rlock() -- try a shared lock, if it fails return immediately
 * * timed_lock() -- try an exclusive lock for the specified amount of time
 * * timed_rlock() -- try a shared lock for the specified amount of time
 * * dated_lock() -- try to obtain an exclusive lock by the specified date
 * * dated_rlock() -- try to obtain a shared lock by the specified date
 * * unlock() -- unlock a locked recursive_rwlock object
 */


// self
//
#include    "cppthread/recursive_rwlock.h"

#include    "cppthread/exception.h"
#include    "cppthread/guard.h"
#include    "cppthread/log.h"
#include    "cppthread/mutex.h"


// snapdev
//
#include    <snapdev/timespec_ex.h>


// C
//
#include    <string.h>


// last include
//
#include    <snapdev/poison.h>




namespace cppthread
{



namespace detail
{



/** \brief The implementation of the recursive_rwlock.
 *
 * We have an internal implementation of the recursive_rwlock to avoid
 * leaking the pthread library to the outside.
 *
 * The class holds a rwlock and metadata to allow for recursivity.
 */
class recursive_rwlock_impl
{
public:
    typedef std::map<pid_t, std::uint32_t>      reader_count_t;

                                    recursive_rwlock_impl(bool prefer_reader);
                                    ~recursive_rwlock_impl();

    bool                            if_locked_by_writer_inc();
    void                            start_lock_as_writer();

    bool                            if_locked_by_writer_or_reader_inc();
    void                            start_lock_as_reader();

    bool                            need_unlock();
    bool                            own_lock() const;
    bool                            own_rlock() const;
    bool                            is_locked() const;
    bool                            is_exclusively_locked() const;
    bool                            is_shared_locked() const;

    void                            lock();
    bool                            try_lock();
    bool                            dated_lock(timespec const & date);

    void                            rlock();
    bool                            try_rlock();
    bool                            dated_rlock(timespec const & date);

    void                            unlock();

private:
    pthread_rwlock_t                f_rwlock = pthread_rwlock_t();
    mutable cppthread::mutex        f_recursive_mutex = cppthread::mutex();
    pid_t                           f_writer_thread = 0;
    std::uint32_t                   f_writer_recursion_count = 0;
    reader_count_t                  f_reader_recursion_counts = reader_count_t();
};


recursive_rwlock_impl::recursive_rwlock_impl(bool prefer_reader)
{
    pthread_rwlockattr_t lattr;
    int err(pthread_rwlockattr_init(&lattr));
    if(err != 0)
    {
        log << log_level_t::fatal
            << "a rwlock attribute structure could not be initialized, error #"
            << err
            << " -- "
            << strerror(err)
            << end;
        throw invalid_error("pthread_muteattr_init() failed");
    }
    err = pthread_rwlockattr_setkind_np(&lattr, prefer_reader
            ? PTHREAD_RWLOCK_PREFER_READER_NP
            : PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
    if(err != 0)
    {
        log << log_level_t::fatal
            << "a rwlock attribute structure type could not be setup, error #"
            << err
            << " -- "
            << strerror(err)
            << end;
        pthread_rwlockattr_destroy(&lattr);
        throw invalid_error("pthread_muteattr_settype() failed");
    }
    err = pthread_rwlock_init(&f_rwlock, &lattr);
    if(err != 0)
    {
        log << log_level_t::fatal
            << "a rwlock structure could not be initialized, error #"
            << err
            << " -- "
            << strerror(err)
            << end;
        pthread_rwlockattr_destroy(&lattr);
        throw invalid_error("pthread_rwlock_init() failed");
    }
    err = pthread_rwlockattr_destroy(&lattr);
    if(err != 0)
    {
        log << log_level_t::fatal
            << "a rwlock attribute structure could not be destroyed, error #"
            << err
            << " -- "
            << strerror(err)
            << end;
        pthread_rwlock_destroy(&f_rwlock);
        throw invalid_error("pthread_rwlockattr_destroy() failed");
    }
}


recursive_rwlock_impl::~recursive_rwlock_impl()
{
    if(is_locked())
    {
        // we cannot legally throw in a destructor so we instead generate a fatal error
        // WARNING: the log << ... may generate a deadlock
        //
        log << log_level_t::fatal
            << "a rwlock is being destroyed when its reference count is "
            << (f_writer_recursion_count + f_reader_recursion_counts.size())
            << " instead of zero."
            << end;
        std::terminate();
    }
    int const err(pthread_rwlock_destroy(&f_rwlock));
    if(err != 0)
    {
        log << log_level_t::fatal
            << "a rwlock destruction generated error #"
            << err
            << " -- "
            << strerror(err)
            << end;
    }
}


bool recursive_rwlock_impl::if_locked_by_writer_inc()
{
    pid_t const tid(gettid());

    cppthread::guard lock(f_recursive_mutex);
    if(f_writer_thread == tid)
    {
        // this thread already owns the writer lock
        //
        ++f_writer_recursion_count;
        return true;
    }

    return false;
}


void recursive_rwlock_impl::start_lock_as_writer()
{
    pid_t const tid(gettid());

    cppthread::guard lock(f_recursive_mutex);
    f_writer_thread = tid;
    f_writer_recursion_count = 1;
}


bool recursive_rwlock_impl::if_locked_by_writer_or_reader_inc()
{
    pid_t const tid(gettid());

    cppthread::guard lock(f_recursive_mutex);
    if(f_writer_thread == tid)
    {
        // this thread already owns the writer lock
        //
        ++f_writer_recursion_count;
        return true;
    }

    auto it(f_reader_recursion_counts.find(tid));
    if(it != f_reader_recursion_counts.end())
    {
        // this thread already owns a reader lock
        //
        ++it->second;
        return true;
    }

    return false;
}


void recursive_rwlock_impl::start_lock_as_reader()
{
    pid_t const tid(gettid());

    cppthread::guard lock(f_recursive_mutex);
    f_reader_recursion_counts[tid] = 1;
}


bool recursive_rwlock_impl::need_unlock()
{
    pid_t const tid(gettid());

    cppthread::guard lock(f_recursive_mutex);

    if(f_writer_thread == tid)
    {
        --f_writer_recursion_count;
        if(f_writer_recursion_count == 0)
        {
            f_writer_thread = 0;
            return true;
        }
        return false;
    }

    auto it(f_reader_recursion_counts.find(tid));
    if(it == f_reader_recursion_counts.end())
    {
        log << log_level_t::fatal
            << "attempting to unlock a rwlock when it is not currently locked."
            << end;
        throw logic_error("lock not found as a reader nor a writer lock."); // LCOV_EXCL_LINE
    }
    if(it->second == 0)
    {
        log << log_level_t::fatal
            << "reader lock found but counter is already zero."
            << end;
        throw logic_error("lock counter is already zero before decrementing it."); // LCOV_EXCL_LINE
    }

    --it->second;
    if(it->second > 0)
    {
        return false;
    }
    f_reader_recursion_counts.erase(it);

    return true;
}


bool recursive_rwlock_impl::own_lock() const
{
    pid_t const tid(gettid());

    cppthread::guard lock(f_recursive_mutex);
    return f_writer_thread == tid;
}


bool recursive_rwlock_impl::own_rlock() const
{
    pid_t const tid(gettid());

    cppthread::guard lock(f_recursive_mutex);
    return f_reader_recursion_counts.contains(tid);
}


bool recursive_rwlock_impl::is_locked() const
{
    cppthread::guard lock(f_recursive_mutex);
    return f_writer_recursion_count != 0
        || !f_reader_recursion_counts.empty();
}


bool recursive_rwlock_impl::is_exclusively_locked() const
{
    cppthread::guard lock(f_recursive_mutex);
    return f_writer_recursion_count != 0;
}


bool recursive_rwlock_impl::is_shared_locked() const
{
    cppthread::guard lock(f_recursive_mutex);
    return !f_reader_recursion_counts.empty();
}


void recursive_rwlock_impl::lock()
{
    if(if_locked_by_writer_inc())
    {
        return;
    }
    if(own_rlock())
    {
        log << log_level_t::error
            << "a rwlock::lock() cannot upgrade a previous shared lock to an exclusive lock."
            << end;
        throw already_locked_for_reading("trying to lock an recursive_rwlock exclusively when already locked in shared mode; lock upgrade is not available.");
    }

    int const err(pthread_rwlock_wrlock(&f_rwlock));
    if(err != 0)
    {
        log << log_level_t::error
            << "a rwlock::lock() generated error #"
            << err
            << " -- "
            << strerror(err)
            << end;
        throw invalid_error("pthread_rwlock_wrlock() failed.");
    }

    start_lock_as_writer();
}


bool recursive_rwlock_impl::try_lock()
{
    if(if_locked_by_writer_inc())
    {
        return true;
    }

    int const err(pthread_rwlock_trywrlock(&f_rwlock));
    if(err == 0)
    {
        start_lock_as_writer();
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
        << "a rwlock try write lock generated error #"
        << err
        << " -- "
        << strerror(err)
        << end;
    throw invalid_error("pthread_rwlock_trywrlock() failed");
}


bool recursive_rwlock_impl::dated_lock(timespec const & date)
{
    if(if_locked_by_writer_inc())
    {
        return true;
    }

    int const err(pthread_rwlock_timedwrlock(&f_rwlock, &date));
    if(err == 0)
    {
        start_lock_as_writer();
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
        << "a rwlock timed write lock generated error #"
        << err
        << " -- "
        << strerror(err)
        << end;
    throw invalid_error("pthread_rwlock_timedwrlock() failed");
}


void recursive_rwlock_impl::rlock()
{
    if(if_locked_by_writer_or_reader_inc())
    {
        return;
    }

    int const err(pthread_rwlock_rdlock(&f_rwlock));
    if(err != 0)
    {
        log << log_level_t::error
            << "a rwlock::rlock() generated error #"
            << err
            << " -- "
            << strerror(err)
            << end;
        throw invalid_error("pthread_rwlock_rdlock() failed");
    }

    start_lock_as_reader();
}


bool recursive_rwlock_impl::try_rlock()
{
    if(if_locked_by_writer_or_reader_inc())
    {
        return true;
    }

    int const err(pthread_rwlock_tryrdlock(&f_rwlock));
    if(err == 0)
    {
        start_lock_as_reader();
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
        << "a rwlock try read lock generated error #"
        << err
        << " -- "
        << strerror(err)
        << end;
    throw invalid_error("pthread_rwlock_tryrdlock() failed");
}


bool recursive_rwlock_impl::dated_rlock(timespec const & date)
{
    if(if_locked_by_writer_or_reader_inc())
    {
        return true;
    }

    int const err(pthread_rwlock_timedrdlock(&f_rwlock, &date));
    if(err == 0)
    {
        start_lock_as_reader();
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
        << "a rwlock try read lock generated error #"
        << err
        << " -- "
        << strerror(err)
        << end;
    throw invalid_error("pthread_rwlock_tryrdlock() failed");
}


void recursive_rwlock_impl::unlock()
{
    // we cannot unlock if it was not locked before!
    //
    if(!need_unlock())
    {
        return;
    }

    int const err(pthread_rwlock_unlock(&f_rwlock));
    if(err != 0)
    {
        log << log_level_t::fatal
            << "a rwlock unlock generated error #"
            << err
            << " -- "
            << strerror(err)
            << end;
        throw invalid_error("pthread_rwlock_unlock() failed");
    }
}



/** \var recursive_rwkock_impl::f_rwlock
 * \brief RWLock to support guards & signals.
 *
 * This is the actual system rwlock. It is useful to protect areas of code
 * that only one thread is allowed to access in write mode or allow any
 * number of threads to access the same data a read-only mode.
 */


/** \var recursive_rwlock_impl::f_recursive_mutex
 * \brief A mutex used to manage the recursive metadata of the rwlock.
 *
 * The recursive_rwlock needs to track the number of times the lock()
 * and unlock() functions get called to know whether the calling threads
 * owns the lock or not and thus whether it should wait on the lock
 * or just increment/decrement a counter. To safely handling that
 * metadata, this class uses this additional mutex.
 */


/** \var recursive_rwkock_impl::f_writer_thread
 * \brief The thread that owns the write lock.
 *
 * When one of the threads obtains the write lock, its tid is saved in
 * this variable.
 */


/** \var recursive_rwkock_impl::f_write_recursion_count
 * \brief How many times the thread that owns the write lock called lock().
 *
 * When one of the threads obtained the write lock, it counts the number
 * of time it tried to lock that rwlock. The unlock() then decrements that
 * number.
 */


/** \var recursive_rwkock_impl::f_reader_recursion_counts
 * \brief How many times the reader locks were called.
 *
 * When one of the threads obtained a read lock, it counts the number
 * of time it tried to lock that rwlock. The unlock() then decrements that
 * number. This is a per thread counter hence the map. Once a counter
 * goes back to 0, it gets removed from the map.
 */



}



/** \class rwlock
 * \brief A rwlock object to ensures atomicity.
 *
 * This class is used by threads when some data shared by more than
 * one thread is about to be accessed. In most cases it is used with the
 * guard class so it is safe even in the event an exception is raised.
 *
 * \code
 *     {
 *         cppthread::guard lock(f_rwlock, false);
 *
 *         ...
 *     } // <-- auto-unlock here
 * \endcode
 *
 * The true/false flag defines whether the read/write lock is taken
 * exclusively (true) or in shared mode (false).
 *
 * \note
 * At the moment, the rwlock should be considered non-recursive even
 * though a read-only lock is recursive when the prefer_reader flag
 * of the constructor was true. This also means the guard cannot
 * safely re-lock a rwlock. In other words, you can use the unlock()
 * early, but trying to call lock() afterward could create a deadlock.
 *
 * \note
 * The rwlock copy and assignment operators are deleted. We do not allow you
 * to copy a rwlock because that is not a good idea. You probably have an
 * issue in your code if you think you need to make a copy of a rwlock.
 * (Although that our implementation, because of the `f_impl`, would allow for
 * copies, it's still not a good idea.)
 */




/** \brief An inter-thread rwlock to ensure unicity of execution.
 *
 * The rwlock object is used to lock part of the code that needs to be run
 * by only one thread (exclusive mode allowing read and write work) or
 * to be run by any number of threads when reading the data only
 * (read-only mode).
 *
 * In most cases one uses the guard object to temporarily lock
 * the rwlock using the stack to help ensure the rwlock gets unlocked as
 * required in the event an exception occurs.
 *
 * \code
 * {
 *    cppthread::guard lock(my_rwlock)
 *    ... // protected code
 * }
 * \endcode
 *
 * The lock can be tried to see whether another thread already has the
 * lock and fail if so. See the try_lock() function.
 *
 * \note
 * Care must be used to always initialized a rwlock before it
 * is possibly accessed by more than one thread. This is usually
 * the case in the constructor of your objects.
 *
 * \exception invalid_error
 * If any one of the initialization functions fails, this exception is
 * raised. The function also logs the error.
 */
recursive_rwlock::recursive_rwlock(bool prefer_reader)
    : f_impl(std::make_shared<detail::recursive_rwlock_impl>(prefer_reader))
{
}


/** \brief Clean up a rwlock object.
 *
 * This function ensures that the rwlock object is cleaned up, which means
 * the rwlock gets destroyed.
 *
 * This destructor verifies that the rwlock is not currently locked. A
 * locked rwlock cannot be safely destroyed. If still locked, then an
 * error is sent to the logger and the function calls std::terminate().
 */
recursive_rwlock::~recursive_rwlock()
{
}


/** \brief Check whether the lock is currently locked.
 *
 * This function checks whether the lock has at least one item
 * locked. This may be by any thread in exclusive or shared
 * mode.
 *
 * \return true if at least one lock is active.
 */
bool recursive_rwlock::is_locked() const
{
    return f_impl->is_locked();
}


bool recursive_rwlock::is_exclusively_locked() const
{
    return f_impl->is_exclusively_locked();
}


bool recursive_rwlock::is_shared_locked() const
{
    return f_impl->is_shared_locked();
}


/** \brief Check whether the current thread owns the exclusive lock.
 *
 * The recursive rwlock may own this lock exclusively (in write mode).
 * This function returns true if that's the case.
 *
 * \return true if the calling thread owns this lock exclusively.
 */
bool recursive_rwlock::own_lock() const
{
    return f_impl->own_lock();
}


/** \brief Exclusively lock a rwlock.
 *
 * This function locks the rwlock in exclusive mode (also called write mode).
 * The function waits until the rwlock is available if it is not currently
 * available. To avoid waiting one may want to use the try_lock() function
 * instead.
 *
 * \exception invalid_error
 * If the lock fails, this exception is raised.
 */
void recursive_rwlock::lock()
{
    f_impl->lock();
}


/** \brief Try locking the rwlock in exclusive mode.
 *
 * This function tries locking the rwlock in exclusive mode. If the rwlock
 * cannot be locked because another thread already locked it, then the
 * function returns immediately with false.
 *
 * \exception invalid_error
 * If the lock fails, this exception is raised.
 *
 * \return true if the lock succeeded, false otherwise.
 */
bool recursive_rwlock::try_lock()
{
    return f_impl->try_lock();
}


/** \brief Time limited attempt at locking the rwlock in exclusive mode.
 *
 * This function tries locking the rwlock in exclusive mode. It waits
 * at most \p wait seconds before giving up.
 *
 * \exception invalid_error
 * If the lock fails, this exception is raised.
 *
 * \param[in] wait  The amount of time to wait before giving up.
 *
 * \return true if the lock succeeded, false otherwise.
 */
bool recursive_rwlock::timed_lock(timespec const & wait)
{
    return f_impl->dated_lock(snapdev::now() + wait);
}


/** \brief Time limited attempt at locking the rwlock in exclusive mode.
 *
 * This function tries locking the rwlock in exclusive mode. It waits
 * until \p date is past before giving up.
 *
 * \exception invalid_error
 * If the lock fails, this exception is raised.
 *
 * \param[in] date  The absolute date when the function returns false if
 *                  it could not obtain the lock.
 *
 * \return true if the lock succeeded, false otherwise.
 */
bool recursive_rwlock::dated_lock(timespec const & date)
{
    return f_impl->dated_lock(date);
}


/** \brief Check whether the current thread owns a read-only (shared) lock.
 *
 * This function checks whether the current thread (caller) owns a
 * read-only lock on this recursive rwlock. The read-only type of
 * lock means the data cannot be modified until the lock is released.
 *
 * If you want to know whether the resource is currently locked,
 * use the is_locked() function instead. This is thread agnostic.
 *
 * \return true if the caller thread has a read-only lock on this
 *         recursive_rwlock.
 */
bool recursive_rwlock::own_rlock() const
{
    return f_impl->own_rlock();
}


/** \brief Lock a rwlock in shared mode.
 *
 * This function locks the rwlock in shared mode (also called read mode).
 * The function waits until the rwlock is available if it is not currently
 * available. To avoid waiting one may want to use the try_lock() function
 * instead.
 *
 * \warning
 * If the constructor was called with prefer_reader set to false, then
 * this call is not recursive. In other words, one thread can call this
 * function one time, then it has to call unlock() before it can call it
 * again.
 *
 * \exception invalid_error
 * If the lock fails, this exception is raised.
 */
void recursive_rwlock::rlock()
{
    f_impl->rlock();
}


/** \brief Try locking the rwlock in shared mode.
 *
 * This function tries locking the rwlock in shared mode. If the rwlock
 * cannot be locked because another thread already locked it, then the
 * function returns immediately with false.
 *
 * \exception invalid_error
 * If the lock fails, this exception is raised.
 *
 * \return true if the lock succeeded, false otherwise.
 */
bool recursive_rwlock::try_rlock()
{
    return f_impl->try_rlock();
}


/** \brief Timed limited locking the rwlock in shared mode.
 *
 * This function tries locking the rwlock in shared mode. It waits
 * at most \p wait seconds before giving up.
 *
 * \exception invalid_error
 * If the lock fails, this exception is raised.
 *
 * \param[in] wait  The amount of time to wait.
 *
 * \return true if the lock succeeded, false otherwise.
 */
bool recursive_rwlock::timed_rlock(timespec const & wait)
{
    return f_impl->dated_rlock(snapdev::now() + wait);
}


/** \brief Time limited attempt at locking the rwlock in shared mode.
 *
 * This function tries locking the rwlock in shared mode. It waits
 * until \p date is past before giving up.
 *
 * \exception invalid_error
 * If the lock fails, this exception is raised.
 *
 * \param[in] date  The absolute date when the function returns false if
 *                  it could not obtain the lock.
 *
 * \return true if the lock succeeded, false otherwise.
 */
bool recursive_rwlock::dated_rlock(timespec const & date)
{
    return f_impl->dated_rlock(date);
}


/** \brief Unlock a rwlock.
 *
 * This function unlocks the specified rwlock. The function must be called
 * exactly once per call to the lock() or rlock() functions, or successful
 * call to the try_lock() try_rlock() functions.
 *
 * The unlock never waits.
 *
 * It is safer to make use of the cppthread::guard to ensure the same
 * number of lock() and unlock() to a rwlock.
 *
 * \warning
 * Keep in mind that these locks are not exactly recursive. If you keep
 * the default mode, i.e. read has priority over write, then the shared
 * locks are recursive, but but the writes never are.
 *
 * \exception invalid_error
 * If the unlock fails, this exception is raised.
 *
 * \exception not_locked
 * If the function is called too many times, then the lock count is going
 * to be zero and this exception will be raised.
 */
void recursive_rwlock::unlock()
{
    f_impl->unlock();
}




/** \typedef recursive_rwlock::pointer_t
 * \brief Shared pointer to a rwlock.
 *
 * You can allocate rwlocks as global variables, local variables, variable
 * members and once in a while you may want to allocate them. In that
 * case, we suggest that you use a shared pointer.
 */


/** \typedef recursive_rwlock::vector_t
 * \brief A vector of rwlocks.
 *
 * This type defines a vector that can be used to manage rwlocks. In
 * this case, the rwlocks must be allocated.
 */


/** \var recursive_rwlock::f_impl
 * \brief The pthread rwlock implementation.
 *
 * This variable member holds the actual pthread rwlock. The rwlock
 * implementation manages this field and others as required.
 */




} // namespace cppthread
// vim: ts=4 sw=4 et
