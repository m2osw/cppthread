// Copyright (c) 2013-2025  Made to Order Software Corp.  All Rights Reserved
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
 * \brief Documentation of the worker.h file.
 *
 * The worker.h file is a template so we document that template here.
 *
 * The worker class is used along the pool and the item_with_predicate
 * to run a specific number of threads to process work in parallel.
 */

#error "Documentation only file, do not compile."

namespace cppthread
{



/** \class worker
 * \brief A runner augmentation allowing for worker threads.
 *
 * This class allows you to create a pool of worker threads. This is
 * useful to add/remove work in a fifo object and have any one
 * worker thread pick up the next load as soon as it becomes
 * available. This is pretty much the fastest way to get work done
 * using threads, however, it really only works if you can easily
 * break down the work by chunk.
 *
 * One pool of worker threads is expected to share one pair of fifo
 * objects. Also. the input and output fifo objects must be of the
 * same type. The output can be set to nullptr if you create all the
 * workloads ahead of time or create new items in the current threads.
 * Both methods will work fine.
 *
 * To use your pool of threads, all you have to do is add data to the
 * input fifo and grab results from the output fifo. Note that
 * the output fifo of one pool of threads can be the input fifo
 * of another pool of threads.
 */


/** \fn worker::worker<T>(std::string const & name , std::size_t position , typename fifo<T>::pointer_t in , typename fifo<T>::pointer_t out)
 * \brief Initialize a worker thread.
 *
 * This function initializes a worker thread. The name should be
 * different for each worker, although there is no test to verify
 * that.
 *
 * The \p in and \p out parameters are pointers to fifo objects used to
 * receive work and return work that was done. The output is optional
 * (you can pass a null pointer). The input can be a class derived
 * from the item_with_predicate if you want to allow dependencies
 * (i.e. prevent a thread to work on a certain item until another
 * thread is done with a certain number of other packets).
 *
 * Keep in mind that data added and removed from the fifo
 * is being copied. It is suggested that you make use of a
 * shared_ptr<> to an object and not directly an object. This
 * is likely to make the push/pop more effective.
 *
 * \param[in] name  The name of this new worker thread.
 * \param[in] position  The worker thread position.
 * \param[in] in  The input FIFO.
 * \param[in] out  The output FIFO.
 */


/** \fn worker::worker<T>(worker<T> const & rhs)
 * \brief Deleted copy operator.
 *
 * The copy operator is deleted to clearly prevent copying of workers.
 * Threads could not easily be duplicated, especially the PC, stack, local
 * variables...
 *
 * \param[in] rhs  The right hand side item.
 */


/** \fn worker<T>::operator = (worker<T> const & rhs)
 * \brief Deleted assignment operator.
 *
 * The assignment operator is deleted to clearly prevent copying of workers.
 * Threads could not easily be duplicated, especially the PC, stack, local
 * variables...
 *
 * \param[in] rhs  The right hand side item.
 *
 * \return a reference to this.
 */


/** \fn worker<T>::position() const
 * \brief Get the worker thread position.
 *
 * Whenever the cppthread_pool class creates a worker thread,
 * it assigns a position to it. The position is not used, but
 * it may help you when you try to debug the system.
 *
 * \return This worker thread position in the cppthread_pool vector.
 */


/** \fn worker<T>::is_working() const
 * \brief Check whether this specific worker thread is busy.
 *
 * This function let you know whether this specific worker thread
 * picked a work load and is currently processing it. The processing
 * includes copying the data to the output FIFO. However, there is
 * a small period of time between the time another work load object
 * is being picked up and the time it gets used that the thread is
 * not marked as working yet. So in other words, this function may
 * be lying at that point.
 *
 * \return Whether the thread is still working.
 */


/** \fn worker<T>::runs()
 * \brief Number of time this worker got used.
 *
 * This function returns the number of time this worker ended up
 * running against a work load.
 *
 * The function may return 0 if the worker never ran. If you create
 * a large pool of threads but do not have much work, this is not
 * unlikely to happen.
 *
 * \return Number of times this worker ran your do_work() function.
 */


/** \fn worker<T>::run()
 * \brief Implement the worker loop.
 *
 * This function is the overload of the snap_runner run() function.
 * It takes care of waiting for more data and run your process
 * by calling the do_work() function.
 *
 * You may reimplement this function if you need to do some
 * initialization or clean up as follow:
 *
 * \code
 *      virtual void run()
 *      {
 *          // initialize my variable
 *          m_my_var = allocate_object();
 *
 *          snap_worker::run();
 *
 *          // make sure to delete that resource
 *          delete_object(m_my_var);
 *      }
 * \endcode
 */


/** \fn worker<T>::do_work()
 * \brief Worker Function.
 *
 * This function is your worker function which will perform work
 * against the work load automatically retrieved in the run()
 * function.
 *
 * Your load is available in the f_workload variable member.
 * You are free to modify it. The snap_worker object ignores
 * its content. It retrieved it from the input fifo (f_in)
 * and will save it in the output fifo once done (f_out).
 *
 * \return true if the work is considered successful and the
 * workload should be forwarded. On false, the workload does
 * not get forwarded to the output fifo.
 */


/** \typedef worker<T>::work_load_type
 * \brief Type T of the worker.
 *
 * This type can be used to \em extract the worker's type as defined by T.
 */


/** \var worker<T>::f_workload
 * \brief The workload this worker is processing.
 *
 * This is the workload, an item from the input fifo, which this thread
 * is expected to work on.
 *
 * Once the thread returns, that workload will be forwarded to the output
 * fifo is there is one.
 */


/** \var worker<T>::f_in
 * \brief The input fifo.
 *
 * This variable member holds the input fifo. It must be defined since this
 * is the only way a worker thread actually receives work.
 */

/** \var worker<T>::f_out
 * \brief The output fifo.
 *
 * This variable member holds the output fifo pointer.
 *
 * The pointer can be set to nullptr in which case the workload gets dropped
 * at the time the thread returns from its do_work() function.
 */


/** \var worker<T>::f_position
 * \brief The position of this worker in the pool.
 *
 * The pool object creates the workers and assigns a position to each one
 * of them. This can be useful in various situations such as while debugging
 * your software.
 */


/** \var worker<T>::f_working
 * \brief Whether this worker is currently working (true) or not (false).
 *
 * The worker sets this flag to true before calling the do_work(). It
 * then sets it back to false on return.
 *
 * \note
 * At the moment, the flag remains set to true if an exception occurs.
 * This is on purpose because that state is not expected.
 */


/** \var worker<T>::f_runs
 * \brief The number of times this worker ran.
 *
 * For statistical purposes, you can use the runs() function which
 * returns the number of time this runner was given the chance to
 * work on a workload.
 */


} // namespace cppthread
// vim: ts=4 sw=4 et
