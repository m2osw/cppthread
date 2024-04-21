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
#include    "cppthread/thread.h"

#include    "cppthread/exception.h"
#include    "cppthread/guard.h"
#include    "cppthread/log.h"
#include    "cppthread/runner.h"


// snapdev
//
#include    <snapdev/glob_to_list.h>


// C++
//
#include    <fstream>


// C
//
#include    <signal.h>
#include    <sys/auxv.h>
#include    <sys/stat.h>
#include    <sys/syscall.h>
#include    <sys/sysinfo.h>
#include    <unistd.h>


// last include
//
#include    <snapdev/poison.h>




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
    init();
}


/** \brief Initialize the thread with a shared pointer to the runner.
 *
 * At times you have a shared pointer to the runner, in which case you
 * can directly use that pointer. This function otherwise works exactly
 * like the other accepting a bare pointer. In other words, the thread
 * does not keep a shared pointer of your runner. It is expected that
 * you do not delete your runner before the thread is done with it.
 *
 * \param[in] name  The name of the process.
 * \param[in] runner  The runner (the actual thread) to handle.
 */
thread::thread(std::string const & name, std::shared_ptr<runner> runner)
    : f_name(name)
    , f_runner(runner.get())
{
    init();
}


/** \brief This private function initializes the thread.
 *
 * We can create the thread with a bare pointer or a shared pointer.
 * This function does the remaining of the initialization.
 */
void thread::init()
{
    if(f_runner == nullptr)
    {
        throw cppthread_invalid_error("runner missing in thread() constructor");
    }
    if(f_runner->f_thread != nullptr)
    {
        throw cppthread_in_use_error(
                      "this runner ("
                    + f_name
                    + ") is already in use");
    }

    int err(pthread_attr_init(&f_thread_attr));
    if(err != 0)
    {
        log << log_level_t::fatal
            << "the thread attributes could not be initialized, error #"
            << err
            << end;
        throw cppthread_invalid_error("pthread_attr_init() failed");
    }
    err = pthread_attr_setdetachstate(&f_thread_attr, PTHREAD_CREATE_JOINABLE);
    if(err != 0)
    {
        log << log_level_t::fatal
            << "the thread detach state could not be initialized, error #"
            << err
            << end;
        pthread_attr_destroy(&f_thread_attr);
        throw cppthread_invalid_error("pthread_attr_setdetachstate() failed");
    }

    f_runner->f_thread = this;
}


/** \brief Delete a thread object.
 *
 * The destructor of a Snap! C++ thread object ensures that the thread stops
 * running before actually deleting the runner object.
 *
 * Then it destroys the thread attributes and returns.
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
    catch(...)
    {
        // stop() may rethrow any user exception which we have to ignore in
        // a destructor...
    }
    f_runner->f_thread = nullptr;

    int const err(pthread_attr_destroy(&f_thread_attr));
    if(err != 0)
    {
        log << log_level_t::error
            << "the thread attributes could not be destroyed, error #"
            << err
            << end;
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
 * The function then calls the internal_thread().
 *
 * \note
 * The function parameter is a void * instead of thread because that
 * way the function signature matches the signature the pthread_create()
 * function expects.
 *
 * \param[in] system_thread  The thread pointer.
 *
 * \return We return a null pointer, which we do not use because we expect
 *         uses to pass results in a different way (i.e. using the fifo).
 */
void * func_internal_start(void * system_thread)
{
    thread * t(reinterpret_cast<thread *>(system_thread));
    t->internal_thread();
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
void thread::internal_thread()
{
    try
    {
        {
            guard lock(f_mutex);
            f_tid = gettid();
            f_started = true;
            f_mutex.signal();
        }

        std::string name(f_runner->get_name());
        if(!name.empty())
        {
            // make sure to limit the name to 15 characters
            //
            if(name.length() > 15)
            {
                name.resize(15);
            }

            // the pthread_setname_np() allows for the name to be retrieved
            // with its counter part:
            //
            //   pthread_getname_np()
            //
            pthread_setname_np(pthread_self(), name.c_str());

            // but to really change the name in the comm file (and therefore
            // htop, ps, etc.) we further call the set_current_thread_name()
            // function which directly writes to that file
            //
            set_current_thread_name(name);
        }

        // if the enter failed, do not continue
        //
        if(internal_enter())
        {
            if(internal_run())
            {
                internal_leave(leave_status_t::LEAVE_STATUS_NORMAL);
            }
            else
            {
                internal_leave(leave_status_t::LEAVE_STATUS_THREAD_FAILED);
            }
        }
        else
        {
            internal_leave(leave_status_t::LEAVE_STATUS_INITIALIZATION_FAILED);
        }

        // if useful (necessary) it would probably be better to call this
        // function from here; see function and read the "note" section
        // for additional info
        //
        //tcp_client_server::cleanup_on_thread_exit();
    }
    catch(std::exception const & e)
    {
        // keep a copy of the exception
        //
        f_exception = std::current_exception();

        if(f_log_all_exceptions)
        {
            log << log_level_t::fatal
                << "thread internal_thread() got exception: \""
                << e.what()
                << "\", exiting thread now."
                << end;
        }

        internal_leave(leave_status_t::LEAVE_STATUS_INSTRUMENTATION);
    }
    catch(...)
    {
        // ... any other exception terminates the whole process ...
        //
        log << log_level_t::fatal
            << "thread internal_thread() got an unknown exception (a.k.a. non-std::exception), exiting process."
            << end;

        // rethrow, our goal is not to ignore the exception, only to
        // have a log about it
        //
        throw;
    }

    // marked we are done (outside of the try/catch because if this one
    // fails, we have a big problem... (i.e. invalid mutex or more unlocks
    // than locks)
    {
        guard lock(f_mutex);
        f_running = false;
        f_tid = PID_UNDEFINED;
        f_mutex.signal();
    }
}


/** \brief Enter the thread runner.
 *
 * This function signals that the thread runner is about to be entered.
 * This is often used as an initialization function.
 *
 * If the runner::enter() function raises an std::exception, then the
 * function saves that exception for the thread owner, emits a log,
 * and returns false.
 *
 * \note
 * The log is not emitted if set_log_all_exceptions() was called with false.
 *
 * \return true if the runner::enter() function returned as expected.
 */
bool thread::internal_enter()
{
    try
    {
        f_runner->enter();
        return true;
    }
    catch(std::exception const & e)
    {
        // keep a copy of the exception
        //
        f_exception = std::current_exception();

        if(f_log_all_exceptions)
        {
            log << log_level_t::fatal
                << "thread internal_enter() got exception: \""
                << e.what()
                << "\", exiting thread now."
                << end;
        }
    }

    return false;
}


/** \brief Execute the run() function.
 *
 * This function specifically calls the run() function in an exception
 * safe manner.
 *
 * If no exception occurs, the function returns true meaning that everything
 * worked as expected.
 *
 * When an std::exception occurs, the function returns false after saving
 * the exception so it can be reported to this thread owner. (i.e. it gets
 * re-thrown whenever the thread owner joins with the thread).
 *
 * The std::exception can be logged by calling the set_log_all_exceptions()
 * function with true, which is the default (i.e. don't call it with false
 * if you want to get the logs).
 *
 * Other exceptions are ignored (they will be caught by the internal_thread()
 * function).
 *
 * \return true if the run worked as expected; false if an exception was
 * caught.
 */
bool thread::internal_run()
{
    try
    {
        f_runner->run();
        return true;
    }
    catch(std::exception const & e)
    {
        // keep a copy of the exception
        //
        f_exception = std::current_exception();

        if(f_log_all_exceptions)
        {
            log << log_level_t::fatal
                << "thread internal_run() got exception: \""
                << e.what()
                << "\", exiting thread now."
                << end;
        }
    }

    return false;
}


/** \brief Function called when leaving the thread runner.
 *
 * Whenever the thread runner leaves, we want to send a signal to the
 * runner owner through the runner::leave() function. This is the thread
 * function which makes sure that the runner::leave() function get called.
 *
 * The function is called with a status which tells us what failed (i.e.
 * the reason for the call).
 *
 * The function is std::exception safe. Unknown exceptions are ignored here
 * since they will be caught by the internal_thread() function.
 *
 * std::exceptions are reported and either ignored (another exception occurred
 * earlier) or reported back to the thread owner after the owner joins with
 * the thread.
 *
 * \param[in] status  The status when the internal_leave() function gets called.
 */
void thread::internal_leave(leave_status_t status)
{
    try
    {
        f_runner->leave(status);
    }
    catch(std::exception const & e)
    {
        // keep the first exception (i.e. internal_enter() or internal_run()
        // have priority on this one)
        //
        if(f_exception == std::exception_ptr())
        {
            f_exception = std::current_exception();
        }

        log << log_level_t::fatal
            << "thread internal_leave() got exception: \""
            << e.what()
            << "\", exiting thread now."
            << end;
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
        log << log_level_t::warning
            << "the thread is already running"
            << end;
        return false;
    }

    if(!f_runner->is_ready())
    {
        log << log_level_t::warning
            << "the thread runner is not ready"
            << end;
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

        log << log_level_t::error
            << "the thread could not be created, error #"
            << err
            << end;
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
 * not actually forcibly stop the thread. It only turns on a flag (namely
 * it makes the is_stopping() function return true) meaning that the
 * thread should stop as soon as possible. This gives the thread the
 * time necessary to do all necessary cleanup before quitting.
 *
 * The stop function blocks until the thread is done.
 *
 * \warning
 * This function throws the thread exceptions that weren't caught in your
 * run() function. This happens after the thread has completed. The exception
 * is then removed from the thread (i.e. it won't re-throw a second time
 * and a call to get_exception() returns a null pointer).
 *
 * \param[in] callback  A function to call after the thread is marked as
 * stopping but before calling join. Useful to send a signal to the child
 * if you could not have done so earlier.
 */
void thread::stop(std::function<void(thread *)> callback)
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

    if(callback != nullptr)
    {
        callback(this);
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
    // and "lose it" at the same time
    //
    if(f_exception != std::exception_ptr())
    {
        std::exception_ptr e;
        e.swap(f_exception);
        std::rethrow_exception(e);
    }
}


/** \brief Retrieve the thread identifier of this thread.
 *
 * Under Linux, threads are tasks like any others. Each task is given a
 * `pid_t` value. This function returns that `pid_t` for this thread.
 *
 * When the thread is not running this function returns PID_UNDEFINED.
 * Note, however, that the value is set a little after the thread started
 * and cleared a little before the thread exits. This is **not** a good
 * way to know whether the thread is running. Use the is_running() function
 * instead.
 *
 * \return The thread identifier (tid) or PID_UNDEFINED if the thread is
 *         not running.
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


/** \brief Whether to log exceptions caught in the thread.
 *
 * We catch exceptions that happen in your threads. By default, we log them
 * with the basic cppthread log mechanism. If you do not want to have this
 * intermediate logging kick in, you can set this flag to false.
 *
 * This flag is defaulted to true because in many cases you are not joining
 * a thread when the exception occurs. That means your code would never
 * see the exception. Instead, you'd have a dangling process as it is
 * likely to expect things are happening in the thread and if not, it
 * gets stuck. At least, in this way, by default you get a message in your
 * logs.
 *
 * \note
 * Unknown exceptions (i.e. those not derived from std::exception), are
 * always logged and re-thrown (which has the effect of terminating your
 * process). Those exceptions are always logged, whatever the state of
 * this "log all exceptions" flag.
 *
 * \param[in] log_all_exceptions  Whether to log (true) all exceptions or
 * not (false).
 *
 * \sa get_log_all_exceptions()
 */
void thread::set_log_all_exceptions(bool log_all_exceptions)
{
    f_log_all_exceptions = log_all_exceptions;
}


/** \brief Retrieve whether all exceptions get logged or not.
 *
 * This function returns true if all exceptions are to be logged. By default
 * this flag is set to true. You can change it with the
 * set_log_all_exceptions() function.
 *
 * Internally, the flag is used to know whether we should log exceptions.
 * It is very useful to do so if you do not join your thread on a constant
 * basis (i.e. in a service where a thread runs as long as the service
 * itself--you probably will want to know when the thread runs in a problem).
 *
 * This also means you do not have to log the exception yourself unless you
 * need some special handling to indicate all the possible messages and
 * parameters found in the exception object. However, the logging doesn't
 * prevent the system from re-throwing the exception once the thread was
 * joined. In other words, whether you log the exception or not, you most
 * certainly want to catch it or your process will be terminated.
 *
 * \return true if the thread object is to log all exceptions.
 *
 * \sa set_log_all_exceptions()
 */
bool thread::get_log_all_exceptions() const
{
    return f_log_all_exceptions;
}


/** \brief Get the exception pointer.
 *
 * When the thread runner raises an exception, it gets saved in the
 * thread object. That exception can be retrieved using this get_exception()
 * function.
 *
 * If no exception occurred, then this pointer will be the nullptr. If
 * an exception did occur, then it will be a pointer to that exception.
 * It can be rethrown inside a try/catch in order to handle it.
 *
 * \code
 *     std::exception_ptr e(my_thread->get_exception());
 *     if(e != std::exception_ptr())
 *     {
 *         try
 *         {
 *             e->rethrow_exception();
 *         }
 *         catch(std::exception const & e)
 *         {
 *             std::cerr << "exception occurred: " << e.what() << std::endl;
 *         }
 *     }
 * \endcode
 *
 * \return A pointer to a standard exception (std::exception).
 */
std::exception_ptr thread::get_exception() const
{
    guard lock(f_mutex);
    return f_exception;
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
 * the signalfd() function to transform SIGUSR1 into a poll-able signal.)
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
 * If your thread does really intensive work for a while (i.e. one thread
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


/** \brief Get the maximum process identifier.
 *
 * This function retrieves the maximum that getpid() may return.
 *
 * The value is cached by the function (in a static variable.) Note that
 * is somewhat wrong since that number can be changed dynamically,
 * although I've seen too many people ever doing so. If your process
 * depends on it, then stop your process, make the change, and
 * restart your process.
 *
 * Note that this function returns the maximum that getpid() can return
 * and not the maximum + 1. In other words, the value returned by this
 * function is inclusive (i.e. in most cases you will get 32767 which a
 * process can have as its PID.)
 *
 * So far, the documentation I've found about the value in the kernel
 * file is not clear about whether that value is inclusive or the
 * last possible PID + 1. I wrote a small test to get the answer and
 * each time the maximum PID I could get was 32767 when the content of
 * "/proc/sys/kernel/pid_max" returns 32768. This is how most C software
 * functions so I am pretty sure our function here is correct.
 *
 * \note
 * The following code often breaks with a fork() failed error. Once
 * you reach the rollover point, though, it cleanly stops on its own.
 * It will print the PID just before the rollover and just after.
 * For example, I get:
 *
 * \code
 *   pid = 32765
 *   pid = 32766
 *   pid = 32767
 *   pid = 301
 * \endcode
 *
 * Of course, if you start this process with the smallest possible
 * PID (such as 301) it will not stop on its own unless the fork()
 * fails which is very likely anyway.
 *
 * \code
 * int main ()
 * {
 *     pid_t pid;
 * 
 *     pid_t start = getpid();
 *     for(int i(0);; ++i)
 *     {
 *         pid = fork();
 *         if(pid == 0)
 *         {
 *             exit(0);
 *         }
 *         if(pid == -1)
 *         {
 *             std::cerr << "fork() failed...\n";
 *             exit(1);
 *         }
 *         std::cerr << "pid = " << pid << "\n";
 *         if(pid < start)
 *         {
 *             break;
 *         }
 *         pthread_yield();
 *     }
 * 
 *     return 0;
 * }
 * \endcode
 *
 * \note
 * We use this function in snaplock which is affected in case the
 * parameter get dynamically changed by writing to
 * "/proc/sys/kernel/pid_max".
 *
 * \return The maximum getpid() can return or -1 if it can't be determined.
 */
pid_t get_pid_max()
{
    // here we cache the result (because this value is so unlikely to
    // ever change--but really we should watch for changes?)
    //
    static pid_t pid_max = 0;

    if(pid_max == 0)
    {
        std::ifstream in;
        in.open("/proc/sys/kernel/pid_max", std::ios::in | std::ios::binary);
        if(in.is_open())
        {
            char buf[32];
            in.getline(buf, sizeof(buf) - 1);
            buf[sizeof(buf) - 1] = '\0';
            pid_max = std::stol(buf);
        }
    }

    return pid_max - 1;
}


/** \brief Set the name of the currently running thread.
 *
 * This function is used to set the name of the currently running thread.
 *
 * \param[in] name  The name of the thread.
 *
 * \return 0 if the function worked, -1 on error.
 */
int set_current_thread_name(std::string const & name)
{
    return set_thread_name(gettid(), name);
}


/** \brief Set the name of the specified thread.
 *
 * This function changes the name of one of your threads. In
 * most cases, you should let the runner set the name of the thread
 * on construction.
 *
 * This function should be used only if the name somehow changes over
 * time, for example:
 *
 * The thread is a worker thread and you want the name to reflect what
 * the worker is currently doing. However, do that only if the worker
 * is going to do work for a while, otherwise it's likely going to be
 * rather useless (i.e. htop refresh rate is 1 to 2 seconds).
 *
 * \exception cppthread_invalid_error
 * This exception is raised if the name is empty.
 *
 * \exception cppthread_out_of_range
 * This exception is raised if the name is too long. Linux limits the
 * name of a thread to 15 characters (the buffer has a limit of
 * 16 characters).
 *
 * \param[in] tid  The thread identifier (its number, not pthread_t).
 * \param[in] name  The name of the thread.
 *
 * \return 0 if the function worked, -1 on error.
 */
int set_thread_name(pid_t tid, std::string const & name)
{
    if(name.empty())
    {
        throw cppthread_invalid_error("thread name cannot be empty.");
    }
    if(name.length() > 15)
    {
        throw cppthread_out_of_range(
              "thread name is limited to 15 characters, \""
            + name
            + "\" is too long.");
    }

    // this is what I had in the process implementation, but it only
    // can set the current "process" (thread really) and it doesn't
    // actually work (that is, the PR_GET_NAME works, but in ps -ef
    // or htop, we do not see the change)
    //
    //if(name != nullptr
    //&& *name != '\0')
    //{
    //    prctl(PR_SET_NAME, name);
    //}

    std::ofstream comm("/proc/" + std::to_string(tid) + "/comm");
    comm << name;

    return comm ? 0 : -1;
}


/** \brief Retrieve the name of the current thread.
 *
 * This function reads the name of the currently running thread by reading
 * its `/proc/\<tid>/comm` file. See the get_thread_name() for more details.
 *
 * \return The name of the current thread.
 *
 * \sa get_thread_name()
 * \sa gettid()
 */
std::string get_current_thread_name()
{
    return get_thread_name(gettid());
}


/** \brief Retrieve the name of a thread.
 *
 * This function reads the name of the \p tid thread by reading its
 * `/proc/\<tid>/comm` file.
 *
 * This function is different from the pthread_getname_np() which will
 * return the in memory name. If you have access to your runner, you
 * may instead want to use its get_name() function. Note that the name
 * of a runner cannot be changed so it may be considered more authoritative.
 *
 * \param[in] tid  The thread identifier (its number, not pthread_t).
 *
 * \return The name of the specified thread.
 *
 * \sa get_current_thread_name()
 * \sa gettid()
 */
std::string get_thread_name(pid_t tid)
{
    std::ifstream comm("/proc/" + std::to_string(tid) + "/comm");
    std::string name;
    comm >> name;
    return name;
}


/** \brief Retrieve the list of threads for a given process.
 *
 * This function reads the list of PID from `/proc/<pid>/task/\*` which is
 * the list of threads of a process has. If only one PID, then this is the
 * main process PID and the process has no threads currently running.
 *
 * \param[in] pid  The parent process to be searched for threads.
 *
 * \return A vector of PIDs.
 */
process_ids_t get_thread_ids(pid_t pid)
{
    process_ids_t results;

    if(pid == -1)
    {
        pid = getpid();
    }

    std::string pattern("/proc/");
    pattern += std::to_string(pid);
    pattern += "/task/*";

    snapdev::glob_to_list<std::vector<std::string>> glob;
    if(glob.read_path<
              snapdev::glob_to_list_flag_t::GLOB_FLAG_IGNORE_ERRORS
            , snapdev::glob_to_list_flag_t::GLOB_FLAG_ONLY_DIRECTORIES>(pattern))
    {
        for(auto s : glob)
        {
            pid_t id(0);
            std::string::size_type pos(s.rfind('/') + 1);

            bool valid(pos < s.length());
            for(; pos < s.length(); ++pos)
            {
                char const c(s[pos]);
                if(c >= '0' && c <= '9')
                {
                    id = id * 10 + c - '0';
                }
                else
                {
                    valid = false;
                    break;
                }
            }
            if(valid)
            {
                results.push_back(id);
            }
        }
    }

    return results;
}


/** \brief Check whether a process is running or not.
 *
 * This function checks whether the `/proc/<pid>` directory exists. If so,
 * that means that the process with \p pid is current running.
 *
 * \note
 * Keep in mind that between the time this function checks whether a process
 * is running or not and the time it returns, the given process may have
 * stopped or a new process may have been started.
 * \note
 * Also, security wise, this is not safe except around the time a process
 * quits and even though, on a very heavy system starting new processes all
 * the time, it may re-use the same \p pid very quickly.
 *
 * \param[in] pid  The process identifier to check.
 *
 * \return true if the process is currently running.
 */
bool is_process_running(pid_t pid)
{
    if(pid == getpid())
    {
        // funny guy testing whether he himself is running!?
        //
        return true;
    }

    std::string proc_path("/proc/");
    proc_path += std::to_string(pid);

    struct stat st;
    return stat(proc_path.c_str(), &st) == 0;
}


/** \brief Retrieve the boot UUID.
 *
 * The Linux kernel generates a UUID on a reboot. This allows software to
 * determine whether the computer was rebooted since the last time it was
 * used.
 *
 * On computers where there is no boot identifier, this function returns
 * an empty string.
 *
 * \return The boot UUID as a string or an empty string if not available.
 */
std::string get_boot_id()
{
    std::ifstream in("/proc/sys/kernel/random/boot_id");
    std::string uuid;
    if(in)
    {
        std::getline(in, uuid);
    }
    return uuid;
}


/** \brief Get the number of threads still not joined in this process.
 *
 * This function gets the number of currently \em running threads present
 * in this process. The function is expected to return at least 1 since
 * it includes the currently running thread (i.e. main thread).
 *
 * It can be called from a thread and is dynamic so the returned number
 * can change on each call.
 *
 * \note
 * If the /proc file system is somehow not accessible, then the function
 * may return -1.
 *
 * \return The number of threads or -1 on an error.
 */
std::size_t get_thread_count()
{
    struct stat task;
    if(stat("/proc/self/task", &task) != 0)
    {
        return -1;
    }

    return task.st_nlink - 2;
}


/** \brief Certain functions may be implemented using the vDSO library.
 *
 * The vDSO library is a Linux extension using a small library which
 * gets loaded at the time a new process gets loaded. This library is
 * common to all processes and allows for some functions such as
 * gettimeofday(2) to be implemented without having to do a kernel
 * system call. This improves speed dramatically.
 *
 * The side effect is that it can affect the functionality of some
 * calls such as time(2), because the time function can end up being
 * off by up to 1 second.
 *
 * If this function returns true, then know that your implementation
 * of the time(2) function may not work exactly as expected. In that
 * case, it is important that you consider using clock_gettime(2)
 * instead. That latter function will properly adjust the second
 * if it would otherwise be off.
 *
 * \return true if the vDSO is in use.
 */
bool is_using_vdso()
{
    // the call to getauxval() is only required once; the result can't change
    // within one process run (however, it has to be different on each run
    // to make it secure)
    //
    static auto g_vdso(getauxval(AT_SYSINFO_EHDR));
    return g_vdso != 0;
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


/** \fn thread::thread(thread const & rhs)
 * \brief The copy operator is deleted.
 *
 * The thread object holds a pointer to a runner which is an OS thread.
 * These would be really difficult to _copy_. Instead we prevent the
 * operation altogether.
 *
 * \param[in] rhs  The right hand side.
 */


/** \fn thread::operator = (thread const & rhs)
 * \brief The assignment operator is deleted.
 *
 * The thread object holds a pointer to a runner which is an OS thread.
 * These would be really difficult to _assign_. Instead we prevent
 * the operation altogether.
 *
 * \param[in] rhs  The right hand side.
 *
 * \return A reference to this object.
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


/** \var thread::f_exception
 * \brief An exception pointer.
 *
 * The exception pointer to the exception that was raised in the runner.
 * By default this pointer is null.
 *
 * This pointer is reset back to a null pointer each time the start()
 * is called.
 *
 * This exception must be caught by your function when calling the
 * stop() function. If you don't catch these, then it will stop your
 * process.
 *
 * \note
 * Only standard exceptions can be caught in this way. You should
 * derive all your exception from std::exception (or use the libexcept).
 */


/** \var thread::f_log_all_exceptions
 * \brief Whether the runner exceptions should be logged or not.
 *
 * Whenever an exception occurs in a runner, the exception pointer is
 * saved in the runner so it can be re-thrown after we join with that
 * thread.
 *
 * The main problem here is that certain threads are not joined until
 * we are done with an application. So the exception lingers and there
 * is absolutely no trace of it, especially if you hit Ctlr-C to exit
 * your software, the exception will 100% be lost.
 *
 * Instead, we have this flag to determine whether the exception should
 * be logged at the time it gets caught instead of just saving it in
 * the pointer. By default the flag is set to true which means we will
 * log the exception immediately. This makes it easy to not forget.
 *
 * If you use threads to run a quick process and then return/join
 * (i.e. stop()), then setting this flag to false is okay.
 *
 * \sa get_log_all_exceptions()
 * \sa set_log_all_exceptions()
 */


/** \var thread::f_mutex
 * \brief The thread mutex to guard various functions.
 *
 * The mutex is used whenever a variable that may be accessed by different
 * threads is used. Especially, it makes sure that the variables do not
 * get changed too early or too late (i.e. avoid race conditions).
 *
 * Also the mutex is used to send signals. For example, the thread waits
 * on the runner to start. Once the runner is started, it signals the
 * main thread which can then wake up and return as everything is now
 * well defined.
 */


/** \var thread::f_name
 * \brief The name of this thread.
 *
 * For debug purposes, you can give each one of your threads a different
 * name. It gets saved in this string.
 */


/** \var thread::f_runner
 * \brief The actual thread object.
 *
 * The runner is the object which holds the system thread and runs the
 * commands. We have a separate object because that way we can make sure
 * the runner destructor isn't called while the thread is still running.
 * If that were to happen, then all the virtual functions would be
 * invalid at that point and the system could crash.
 *
 * So the thread object holds a runner allowing the thread to be destroyed
 * first, which calls the stop() function before the runner gets destroyed.
 */


/** \var thread::f_running
 * \brief The thread is running.
 *
 * When the thread is running, this flag is set to true. The start()
 * function sets this flag to true before starting the thread and the
 * internal_run() function sets the flag back to false right before
 * the thread exits.
 *
 * In other words, the flag is true early and false early. It's not
 * a way to know whether the actual system thread is running or not.
 */


/** \var thread::f_started
 * \brief The thread is started.
 *
 * The start() function returns only after the thread is started. To control
 * that state, we use this flag. The caller will wait until the child
 * thread is started (i.e. f_started is true).
 */


/** \var thread::f_stopping
 * \brief The thread is currently in the stopping process.
 *
 * When the stop() function is called, the flag is set to true until after
 * the caller joined with the thread.
 *
 * The is_stopping() function returns the current value of that field.
 */


/** \var thread::f_thread_attr
 * \brief This thread attributes.
 *
 * When we create a thread we create attributes to assign to the thread
 * on creation. These are the attributes.
 *
 * These are created once at the time the thread object is created and
 * released when the object is destroyed. The same attributes are reused
 * to re-start the thread over and over again (i.e. start() / stop()
 * sequences).
 */


/** \var thread::f_thread_id
 * \brief This thread identifier.
 *
 * Each thread is assigned a unique identifier by the C-library. This is
 * that identifier. Under Linux, it is an IP address.
 */


/** \var thread::f_tid
 * \brief This thread identifier.
 *
 * Each thread is assigned a unique identifier by the OS.
 */



/** \var PID_UNDEFINED
 * \brief The value a PID variable is set to when not representing a process.
 *
 * In many cases we want to be able to initialize a pid_t variable with some
 * default value. This variable is generally the best choice.
 *
 * \code
 *     pid_t    f_my_process = PID_UNDEFINED;
 * \endcode
 */


/** \var THREAD_UNDEFINED
 * \brief An invalid thread identifier for initialization.
 *
 * In many cases we want to be able to initialize a pthread_t variable
 * with some default value. This variable is generally the best choice.
 *
 * \code
 *     pthread_t    f_my_thread = THREAD_UNDEFINED;
 * \endcode
 */


/** \typedef process_ids_t
 * \brief A list of identifiers representing various processes.
 *
 * This type defines a vector that can hold identifiers to various
 * processes.
 */



} // namespace cppthread
// vim: ts=4 sw=4 et
