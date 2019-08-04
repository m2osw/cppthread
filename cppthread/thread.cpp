// Snap Websites Server -- C++ object to handle threads
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
#include "cppthread/thread.h"

#include "cppthread/exception.h"
#include "cppthread/guard.h"
#include "cppthread/runner.h"


// advgetopt lib
//
#include "advgetopt/log.h"


// C lib
//
#include <signal.h>
#include <sys/syscall.h>
#include <sys/sysinfo.h>
#include <unistd.h>


// last include
//
#include "snapdev/poison.h"




namespace cppthread
{





/** \brief Initialize the thread object.
 *
 * This function saves the name of the thread. The name is generally a
 * static string and it is used to distinguish between threads when
 * managing several at once. The function makes a copy of the name.
 *
 * The runner pointer is an object which has a run() function that will
 * be called from another thread. That object becomes the "child" of
 * this thread \em controller. However, if it is already assigned a
 * thread controller, then the initialization of the thread fails.
 * You may test whether a runner is already assigned a thread controller
 * by calling its get_thread() function and see that it is not nullptr.
 *
 * The pointer to the runner object cannot be nullptr.
 *
 * \param[in] name  The name of the process.
 * \param[in] runner  The runner (the actual thread) to handle.
 */
thread::thread(std::string const & name, runner * runner)
    : f_name(name)
    , f_runner(runner)
{
    if(f_runner == nullptr)
    {
        throw cppthread_exception_invalid_error("runner missing in thread() constructor");
    }
    if(f_runner->f_thread != nullptr)
    {
        throw cppthread_exception_in_use_error("this runner (" + name + ") is already is use");
    }

    int err(pthread_attr_init(&f_thread_attr));
    if(err != 0)
    {
        advgetopt::log << advgetopt::log_level_t::fatal
                       << "the thread attributes could not be initialized, error #"
                       << err
                       << advgetopt::end;
        throw cppthread_exception_invalid_error("pthread_attr_init() failed");
    }
    err = pthread_attr_setdetachstate(&f_thread_attr, PTHREAD_CREATE_JOINABLE);
    if(err != 0)
    {
        advgetopt::log << advgetopt::log_level_t::fatal
                       << "the thread detach state could not be initialized, error #"
                       << err
                       << advgetopt::end;
        pthread_attr_destroy(&f_thread_attr);
        throw cppthread_exception_invalid_error("pthread_attr_setdetachstate() failed");
    }

    f_runner->f_thread = this;
}


/** \brief Delete a thread object.
 *
 * The destructor of a Snap! C++ thread object ensures that the thread stops
 * running before actually deleting the runner object.
 *
 * Then it destroyes the thread attributes and returns.
 *
 * The destructor also removes the thread from the runner so the runner
 * can create another thread controller and run again.
 */
thread::~thread()
{
    try
    {
        stop();
    }
    catch(cppthread_exception_mutex_failed_error const &)
    {
    }
    catch(cppthread_exception_invalid_error const &)
    {
    }
    f_runner->f_thread = nullptr;

    int const err(pthread_attr_destroy(&f_thread_attr));
    if(err != 0)
    {
        advgetopt::log << advgetopt::log_level_t::error
                       << "the thread attributes could not be destroyed, error #"
                       << err
                       << advgetopt::end;
    }
}


/** \brief Retrieve the name of this process object.
 *
 * This process object is given a name on creation. In most cases this is
 * a static name that is used to determine which process is which.
 *
 * \return The name of the process.
 */
std::string const & thread::get_name() const
{
    return f_name;
}


/** \brief Get a pointer to this thread runner.
 *
 * This function returns the pointer to the thread runner. There are cases
 * where it is quite handy to be able to use this function rather than
 * having to hold on the information in your own way.
 *
 * You will probably have to dynamic_cast<>() the result to your own
 * object type.
 *
 * \note
 * The thread constructor ensures that this pointer is never nullptr.
 * Therefore this function never returns a null pointer. However, the
 * dynamic_cast<>() function may return a nullptr.
 *
 * \return The runner object attached to this thread.
 */
runner * thread::get_runner() const
{
    return f_runner;
}


/** \brief Check whether the thread is considered to be running.
 *
 * This flag is used to know whether the thread is running.
 *
 * \todo
 * We need to save the pid of the process that creates threads.
 * This tells us whether the thread is running in this process.
 * i.e. if a fork() happens, then the child process does not
 * have any threads so is_running() has to return false. Also,
 * any other function that checks whether a thread is running
 * needs to also check the pid. We have to save the pid at
 * the time we start the thread and then when we check whether
 * the thread is running.
 *
 * \return true if the thread is still considered to be running.
 */
bool thread::is_running() const
{
    guard lock(f_mutex);
    return f_running;
}


/** \brief Check whether the thread was asked to stop.
 *
 * The thread is using three status flags. One of them is f_stopping which
 * is set to false (which is also the default status) when start() is called
 * and to true when stop() is called. This function is used to read that
 * flag status from the continue_running() function.
 *
 * \return true if the stop() function was called, false otherwise.
 */
bool thread::is_stopping() const
{
    guard lock(f_mutex);
    return f_stopping;
}


/** \brief Start the actual thread.
 *
 * This function is called when starting the thread. This is a static
 * function since pthread can only accept such a function pointer.
 *
 * The function then calls the internal_run().
 *
 * \note
 * The function parameter is a void * instead of thread because that
 * way the function signature matches the signature the pthread_create()
 * function expects.
 *
 * \param[in] thread  The thread pointer.
 *
 * \return We return a null pointer, which we do not use because we do
 *         not call the pthread_join() function.
 */
void * func_internal_start(void * system_thread)
{
    thread * t(reinterpret_cast<thread *>(system_thread));
    t->internal_run();
    return nullptr; // == pthread_exit(nullptr);
}


/** \brief Run the thread process.
 *
 * This function is called by the func_internal_start() so we run from
 * within the thread class. (i.e. the func_internal_start() function
 * itself is static.)
 *
 * The function marks the thread as started which allows the parent start()
 * function to return.
 *
 * \note
 * The function catches standard exceptions thrown by any of the functions
 * called by the thread. When that happens, the thread returns early after
 * making a copy of the exception. The stop() function will then rethrow
 * that exception and it can be managed as if it had happened in the main
 * process (if a thread creates another thread, then it can be propagated
 * multiple times between all the threads up to the main process.)
 */
void thread::internal_run()
{
    {
        guard lock(f_mutex);
        f_tid = gettid();
    }

    try
    {
        {
            guard lock(f_mutex);
            f_started = true;
            f_mutex.signal();
        }

        f_runner->run();

        // if useful (necessary) it would probably be better to call this
        // function from here; see function and read the "note" section
        // for additional info
        //
        //tcp_client_server::cleanup_on_thread_exit();
    }
    catch(std::exception const &)
    {
        // keep a copy of the exception
        //
        f_exception = std::current_exception();
    }
    catch(...)
    {
        // ... any other exception terminates the whole process ...
        //
        advgetopt::log << advgetopt::log_level_t::fatal
                       << "thread got an unknown exception (a.k.a. non-std::exception), exiting process."
                       << advgetopt::end;

        // rethrow, our goal is not to ignore the exception, only to
        // have a log about it
        //
        throw;
    }

    // marked we are done (outside of the try/catch because if this one
    // fails, we have a big problem... (i.e. invalid mutex or more unlock
    // than locks)
    {
        guard lock(f_mutex);
        f_running = false;
        f_tid = -1;
        f_mutex.signal();
    }
}


/** \brief Attempt to start the thread.
 *
 * This function is used to start running the thread code. If the
 * thread is already running, then the function returns false.
 *
 * The function makes use of a condition to wait until the thread
 * is indeed started. The function will not return until the thread
 * is started or something failed.
 *
 * \return true if the thread successfully started, false otherwise.
 */
bool thread::start()
{
    guard lock(f_mutex);

    if(f_running || f_started)
    {
        advgetopt::log << advgetopt::log_level_t::warning
                       << "the thread is already running"
                       << advgetopt::end;
        return false;
    }

    if(!f_runner->is_ready())
    {
        advgetopt::log << advgetopt::log_level_t::warning
                       << "the thread runner is not ready"
                       << advgetopt::end;
        return false;
    }

    f_running = true;
    f_started = false;
    f_stopping = false; // make sure it is reset
    f_exception = std::exception_ptr();

    int const err(pthread_create(&f_thread_id, &f_thread_attr, &func_internal_start, this));
    if(err != 0)
    {
        f_running = false;

        advgetopt::log << advgetopt::log_level_t::error
                       << "the thread could not be created, error #"
                       << err
                       << advgetopt::end;
        return false;
    }

    while(!f_started)
    {
        f_mutex.wait();
    }

    return true;
}


/** \brief Stop the thread.
 *
 * This function requests the thread to stop. Note that the function does
 * not actually forcebly stop the thread. It only turns on a flag (namely
 * it makes the is_stopping() function return true) meaning that the
 * thread should stop as soon as possible. This gives the thread the
 * time necessary to do all necessary cleanup before quitting.
 *
 * The stop function blocks until the thread is done.
 *
 * \warning
 * This function throws the thread exceptions that weren't caught in your
 * run() function. This happens after the thread has completed.
 */
void thread::stop()
{
    {
        guard lock(f_mutex);

        if(!f_running && !f_started)
        {
            // we return immediately in this case because
            // there is nothing to join when the thread never
            // started...
            //
            return;
        }

        // request the child to stop
        //
        f_stopping = true;
    }

    // wait for the child to be stopped
    //
    // we cannot pass any results through the pthread interface so we
    // pass a nullptr for the result; instead, the user is expected to
    // add fields to his class and fill in whatever results he wants
    // there; it is going to work much better that way
    //
    pthread_join(f_thread_id, nullptr);

    // at this point the thread has fully exited

    // we are done now
    //
    // these flags are likely already the correct value except for
    // f_stopping which the stop() function manages here
    //
    f_running = false;
    f_started = false;
    f_stopping = false;

    // if the child died because of a standard exception, rethrow it now
    //
    if(f_exception != std::exception_ptr())
    {
        std::rethrow_exception(f_exception);
    }
}


/** \brief Retrieve the thread identifier of this thread.
 *
 * Under Linux, threads are tasks like any others. Each task is given a
 * `pid_t` value. This function returns that `pid_t` for this thread.
 *
 * When the thread is not running this function returns -1. Note, however,
 * that the value is set a little after the thread started and cleared a
 * little before the thread exists. This is **not** a good way to know
 * whether the thread is running. Use the is_running() function instead.
 *
 * \return The thread identifier (tid) or -1 if the thread is not running.
 */
pid_t thread::get_thread_tid() const
{
    guard lock(f_mutex);
    return f_tid;
}


/** \brief Retrieve a reference to the thread mutex.
 *
 * This function returns a reference to this thread mutex. Note that
 * the `runner` has its own mutex as well.
 *
 * \return This thread's mutex.
 */
mutex & thread::get_thread_mutex() const
{
    return f_mutex;
}


/** \brief Send a signal to this thread.
 *
 * This function sends a signal to a specific thread.
 *
 * You have to be particularly careful with Unix signal and threads as they
 * do not always work as expected. This is yet particularly useful if you
 * want to send a signal such as SIGUSR1 and SIGUSR2 to a thread so it
 * reacts one way or another (i.e. you are using poll() over a socket and
 * need to be stopped without using a possibly long time out, you can use
 * the signalfd() function to transform SIGUSR1 into a pollable signal.)
 *
 * \note
 * Obviously, if the thread is not running, nothing happens.
 *
 * \param[in] sig  The signal to send to this thread.
 *
 * \return true if the signal was sent, false if the signal could not
 *         be sent (i.e. the thread was already terminated...)
 */
bool thread::kill(int sig)
{
    guard lock(f_mutex);
    if(f_running)
    {
        // pthread_kill() returns zero on success, otherwise it returns
        // an error code which at this point we lose
        //
        return pthread_kill(f_thread_id, sig) == 0;
    }

    return false;
}








/** \brief Retrieve the number of processors available on this system.
 *
 * This function returns the number of processors available on this system.
 *
 * \attention
 * Note that the OS may not be using all of the available processors. This
 * function returns the total number, including processors that are not
 * currently usable by your application. Most often, you probably want to
 * call get_number_of_available_processors() instead.
 *
 * \return The total number of processors on this system.
 *
 * \sa get_number_of_available_processors()
 */
int get_total_number_of_processors()
{
    return get_nprocs_conf();
}


/** \brief Retrieve the number of processors currently usable.
 *
 * This function returns the number of processors that are currently
 * running on this system. This is usually what you want to know about
 * to determine how many threads to run in parallel.
 *
 * This function is going to be equal or less than what the
 * get_total_number_of_processors() returns. For example, on some systems
 * a processors can be made offline. This is useful to save energy when
 * the total load decreases under a given threshold.
 *
 * \note
 * What is the right number of threads needed in a thread pool? If you
 * have worker threads that do a fairly small amount of work, then
 * having a number of threads equal to two times the number of
 * available thread is still sensible:
 *
 * \code
 *     pool_size = get_number_of_available_processors() * 2;
 * \endcode
 *
 * \par
 * This is particularly true when threads perform I/O with high latency
 * (i.e. read/write from a hard drive or a socket.)
 *
 * \par
 * If your thread does really intesive work for a while (i.e. one thread
 * working on one 4Mb image,) then the pool size should be limited to
 * one worker per CPU:
 *
 * \code
 *     pool_size = get_number_of_available_processors();
 * \endcode
 *
 * \par
 * Also, if you know that you will never get more than `n` objects at
 * a time in your input, then the maximum number of threads needed is
 * going to be `n`. In most cases `n` is going to be larger than the
 * number of processors although now that we can have machines with
 * over 100 CPUs, make sure to clamp your `pool_size` parameter to
 * `n` if known ahead of time.
 *
 * \return The number of processors currently available for your threads.
 *
 * \sa get_total_number_of_processors()
 */
int get_number_of_available_processors()
{
    return get_nprocs();
}


/** \brief Get the thread identifier of the current thread.
 *
 * This function retrieves the thread identifier of the current thread.
 * In most cases, this is only useful to print out messages to a log
 * including the thread identifier. This identifier is equivalent
 * to the `pid_t` returned by `getpid()` but specific to the running
 * thread.
 *
 * \return The thread identifier.
 */
pid_t gettid()
{
    return static_cast<pid_t>(syscall(SYS_gettid));
}

































/** \class thread
 * \brief A thread object that ensures proper usage of system threads.
 *
 * This class is used to handle threads. It should NEVER be used, however,
 * there are some very specific cases where a thread is necessary to make
 * sure that main process doesn't get stuck. For example, the process
 * environment using pipes requires threads to read and write pipes
 * because otherwise the processes could lock up.
 */


/** \typedef thread::pointer_t
 * \brief The shared pointer for a thread object.
 *
 * This type is used to hold a smart pointer to a thread.
 *
 * This smart pointer is safe. It can be used to hold a thread object and
 * when it goes out of scope, it properly ends the corresponding thread
 * runner (the runner) and returns.
 *
 * Be cautious because the smart pointer of a runner is not actually
 * safe to delete without first stopping the thread. Make sure to manage
 * all your threads in with two objects, making sure that the thread goes
 * out of scope first so it can stop your thread before your thread object
 * gets destroyed.
 */


/** \typedef thread::vector_t
 * \brief A vector of threads.
 *
 * This type defines a vector of threads. Since each entry in the vector
 * is a smart pointer, it is safe to use this type.
 */






} // namespace snap
// vim: ts=4 sw=4 et
