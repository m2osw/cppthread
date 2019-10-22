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
 * \param[in] name  The name of this thread runner.
 */
runner::runner(std::string const & name)
    : f_name(name)
{
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
 * By default a thread runner is considered ready. If you reimplement this
 * function it is possible to tell the thread controller that you are not
 * ready. This means the start() function will fail and return false.
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
 * This class defines the actuall thread wrapper. This is very important
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
