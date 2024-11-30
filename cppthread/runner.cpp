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
#include    "cppthread/runner.h"

#include    "cppthread/guard.h"
#include    "cppthread/log.h"
#include    "cppthread/thread.h"


// last include
//
#include    <snapdev/poison.h>




namespace cppthread
{



/** \brief Initializes the runner.
 *
 * The constructor expects a name. The name is mainly used in case a
 * problem occur and we want to log a message. That way you will know
 * which thread runner caused a problem.
 *
 * \note
 * The first 15 characters of the name are also used to set the thread
 * name with the pthread_setname_np() and set_current_thread_name()
 * functions. That name is then available in the
 * `/proc/self/task/<tid>/comm` file, which is useful as such a name will
 * appear in the output of ps and htop and other such tools.
 *
 * See: https://stackoverflow.com/questions/68676407/how-do-i-change-the-name-of-one-singlre-thread-in-linux#68676407
 *
 * \param[in] name  The name of this thread runner.
 */
runner::runner(std::string const & name)
    : f_name(name)
{
    // TBD: should we forbid starting a runner without a name?
}


/** \brief The destructor checks that the thread was stopped.
 *
 * This function verifies that the thread was stopped before the
 * object gets destroyed (and is likely to break something along
 * the way.)
 */
runner::~runner()
{
    // the thread should never be set when the runner gets deleted
    if(f_thread)
    {
        // this is a bug; it could be that the object that derived from
        // the snap_runner calls gets destroyed under the thread controller's
        // nose and that could break a lot of things.
        log << log_level_t::fatal
            << "The Snap! thread runner named \""
            << f_name
            << "\" is still marked as running when its object is being destroyed."
            << end;
        std::terminate();
    }
}


/** \brief Retrieve the name of the runner.
 *
 * This function returns the name of the runner as specified in the
 * constructor.
 *
 * Since the name is read-only, it will always match one to one what
 * you passed on.
 *
 * \return The name of this thread runner.
 */
std::string const & runner::get_name() const
{
    return f_name;
}


/** \brief Check whether this thread runner is ready.
 *
 * By default a thread runner is considered ready. If you override this
 * function, it is possible to tell the thread controller that you are not
 * ready. This means the start() function fails and returns false.
 *
 * \return true by default, can return false to prevent a start() command.
 */
bool runner::is_ready() const
{
    return true;
}


/** \brief Whether the thread should continue running.
 *
 * This function checks whether the user who handles the controller asked
 * the thread to quit. If so, then the function returns false. If not
 * the function returns true.
 *
 * The function can be reimplemented in your runner. In that case, the
 * runner implementation should probably call this function too in order
 * to make sure that the stop() function works.
 *
 * It is expected that your run() function implements a loop that checks
 * this flag on each iteration with iterations that take as little time
 * as possible.
 *
 * \code
 * void my_runner::run()
 * {
 *    while(continue_running())
 *    {
 *       // do some work
 *       ...
 *    }
 * }
 * \endcode
 *
 * \return true if the thread is expected to continue running.
 */
bool runner::continue_running() const
{
    guard lock(f_mutex);
    if(f_thread == nullptr)
    {
        return true;
    }
    return !f_thread->is_stopping();
}


/** \brief Signal that the run() function is about to be entered.
 *
 * This function is often used as a way to initialize the thread runner.
 * The default is to log the fact that the thread is being started. You
 * often call it at the start of your enter() implementation.
 *
 * The reason for heaving a separate enter() and leave() pair of functions
 * is to help with the possibility that your run() function throws and is
 * not waited on. If the \em parent thread is working on something else
 * or waiting on a different thread, then you would have no idea that the
 * thread is ending.
 *
 * \attention
 * The enter() will always be called, but the run() and leave() functions
 * do not get called if a preceeding call ends in an abnormal manner (i.e.
 * such as emitting an abort() call, a SEGV, etc.) Exceptions are properly
 * handled, however, if the enter() function exits with an exception, then
 * the run() function doesn't get called.
 */
void runner::enter()
{
    // there is a mutex lock in the get_thread_tid() so try to avoid a
    // potential deadlock by getting the value ahead
    //
    pid_t const tid(f_thread->get_thread_tid());

    log << log_level_t::info
        << "entering thread \""
        << get_name()
        << "\" #"
        << tid
        << "."
        << end;
}


/** \brief Signal that the run() function has returned.
 *
 * This function is called whenever the run() function is done. It may also
 * be called if the enter() function throws in which case the run() function
 * does not get called but the leave() function still gets called.
 *
 * This function is useful to know that the run() function returned without
 * you having to instrument your run() function with a try/catch. This is
 * particularly useful to detect that a thread died when not expected.
 * Specifically, if your \em parent thread is not activelly waiting on
 * your thread demise, the fact that the run() function threw an exception
 * will not be known until later if ever. The leave() function can act in
 * such a way that the \em parent thread is then aware of the issue and
 * either quits, restarts the thread, or just reports the issue.
 *
 * The \p status parameter defines which location the leave() function is
 * called from. It can be called in the following cases:
 *
 * * LEAVE_STATUS_NORMAL -- the enter() and run() functions worked as expected.
 * * LEAVE_STATUS_INITIALIZATION_FAILED -- the enter() function failed with
 * an exception; the run() function was never called.
 * * LEAVE_STATUS_THREAD_FAILED -- the run() function was called and it
 * generated an exception.
 * * LEAVE_STATUS_INSTRUMENTATION -- a function, other than the enter() or
 * run() functions, generated an error.
 *
 * The default function logs the fact that the thread is exiting. You often
 * call it at the end of your own leave() implementation.
 *
 * \param[in] status  The location from which the leave() function get called.
 */
void runner::leave(leave_status_t status)
{
    // there is a mutex lock in the get_thread_tid() so try to avoid a
    // potential deadlock by getting the value ahead
    //
    pid_t const tid(f_thread->get_thread_tid());

    log << log_level_t::info
        << "leaving thread \""
        << get_name()
        << "\" #"
        << tid
        << " with status "
        << static_cast<int>(status)     // TODO: write name too
        << "."
        << end;
}


/** \brief Retrieve the thread controller linked to this runner.
 *
 * Each runner is assigned a thread controller whenever the thread
 * is created (they get attached, in effect.) Once the thread is
 * destroyed, the pointer goes back to nullptr.
 *
 * \return A thread pointer or nullptr.
 */
thread * runner::get_thread() const
{
    return f_thread;
}


/** \brief Get this runner thread identifier.
 *
 * This function returns the thread identifier of the thread running
 * this runner run() function.
 *
 * This function can be called from any thread and the correct value
 * will be returned.
 *
 * \return The thread identifier.
 */
pid_t runner::gettid() const
{
    return f_thread->get_thread_tid();
}



/** \class runner
 * \brief The runner is the class that wraps the actual system thread.
 *
 * This class defines the actual thread wrapper. This is very important
 * because when the main thread object gets destroyed and if it
 * were a system thread, the virtual tables would be destroyed and thus
 * invalid before you reached the ~thread() destructor. This means
 * any of the virtual functions could not get called.
 *
 * For this reason we have a two level thread objects implementation:
 * the thread which acts as a controller and the snap_runner which
 * is the object that is the actual system thread and thus which has the
 * run() virtual function: the function that gets called when the thread
 * starts running.
 */


/** \typedef runner::pointer_t
 * \brief The shared pointer of a thread runner.
 *
 * This type is used to hold a smart pointer to a thread runner.
 *
 * Be very careful. Using a smart pointer does NOT mean that you can just
 * delete a snap_runner without first stopping the thread. Make sure to
 * have a thread object to manage your snap_running pointers (i.e you
 * can delete a thread, which will stop your snap_runner and then
 * delete the snap_runner.)
 */


/** \typedef runner::vector_t
 * \brief A vector of threads.
 *
 * This type defines a vector of thread runners as used by the
 * cppthread::thread_pool template.
 *
 * Be careful as vectors are usually copyable and this one is because it
 * holds smart pointers to thread runners, not the actual thread. You
 * still only have one thread, just multiple instances of its pointer.
 * However, keep in mind that you can't just destroy a runner. The
 * thread it is runner must be stopped first. Please make sure to
 * have a thread or a thread::pool to manage
 * your thread runners.
 */


/** \enum leave_status_t
 * \brief The exit status.
 *
 * When the runner exits, the run() function saves the status of how the
 * function exited. It can be useful to know how the run() function exited
 * to decide on how to react.
 */


/** \var runner::f_mutex
 * \brief The mutex of this thread.
 *
 * Each thread is given its own mutex so it can handle its data safely.
 *
 * This mutex is expected to mainly be used by the thread and its parent.
 *
 * If you want to share data and mutexes between multiple threads,
 * you may want to consider using another mutex. For example, the
 * cppthread::fifo is itself derived from the mutex
 * class. So when you use a FIFO between multiple threads, the
 * lock/unlock mechanism is not using the mutex of your thread.
 */


/** \var runner::f_thread
 * \brief A pointer back to the owner ("parent") of this runner
 *
 * When a snap_runner is created, it gets created by a specific \em parent
 * object. This pointer holds that parent.
 *
 * The runner uses this pointer to know whether it is still running
 * and to retrieve its identifier that the parent holds.
 */


/** \var runner::f_name
 * \brief The name of this thread.
 *
 * Each thread is given a name. This can help greatly when debugging a
 * threaded environment with a large number of threads. That way you
 * can easily identify which thread did what and work you way to a
 * perfect software.
 *
 * On some systems it may be possible to give this name to the OS
 * which then can be displayed in tools listing processes and threads.
 */


/** \fn runner::runner(runner const & rhs);
 * \brief The copy operator is deleted.
 *
 * A runner represents a running thread which is pretty much impossible to
 * copy so we prevent such of the class too.
 *
 * \param[in] rhs  The right hand side.
 */


/** \fn runner::operator = (runner const & rhs);
 * \brief This assignment operator is deleted.
 *
 * A runner represents a running thread which is pretty much impossible to
 * copy so we prevent such of the class too.
 *
 * \param[in] rhs  The right hand side.
 *
 * \return A reference to this object.
 */


/** \fn runner::run();
 * \brief This virtual function represents the code run by the thread.
 *
 * The run() function is the one you have to implement in order to have
 * something to execute when the thread is started.
 *
 * To exit the thread, simply return from the run() function.
 */




} // namespace cppthread
// vim: ts=4 sw=4 et
