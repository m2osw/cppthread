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
 * \brief Documentation of the fifo.h file.
 *
 * The fifo.h file is a template so we document that template here.
 */

#error "Documentation only file, do not compile."

namespace cppthread
{



/** \class pool
 * \brief Manage a pool of worker threads.
 *
 * This function manages a pool of worker threads. It allocates the
 * threads, accepts incoming data, and returns outgoing data. Everything
 * else is managed for you.
 *
 * To make use of this template, you need to overload the snap_worker
 * template and implement your own snap_worker::do_work() function.
 *
 * Something like this:
 *
 * \code
 *      struct data
 *      {
 *          std::string f_func = "counter";
 *          int         f_counter = 0;
 *      };
 *
 *      class foo
 *          : public snap_worker<data>
 *      {
 *          void do_work()
 *          {
 *              if(f_workload.f_func == "counter")
 *              {
 *                  ++f_workload.f_counter;
 *              }
 *              else if(f_workload.f_func == "odd-even")
 *              {
 *                  f_workload.f_counter ^= 1;
 *              }
 *              // ...etc...
 *          }
 *      };
 *
 *      cppthread_pool<foo> pool("my pool", 10);
 *
 *      // generate input
 *      data data_in;
 *      ...fill data_in...
 *      pool.push_back(data_in);
 *
 *      // retrieve output
 *      data data_out;
 *      if(pool.pop_front(data_out))
 *      {
 *          ...use data_out...
 *      }
 * \endcode
 *
 * You can do all the push_data() you need before doing any pop_front().
 * You may also want to consider looping with both interleaved or even
 * have two threads: one feeder which does the push_data() and one
 * consumer which does the pop_front().
 *
 * Note that the thread pool does not guarantee order of processing.
 * You must make sure that each individual chunk of data you pass in
 * the push_front() function can be processed in any order.
 */

/** \class pool::worker_thread_t
 * \brief Class used to manage the worker and worker thread.
 *
 * This class creates a worker thread, it adds it to a thread,
 * and it starts the thread. It is here so we have a single list
 * of _worker threads_.
 *
 * \note
 * I had to allocate a cppthread object because at this point
 * the cppthread is not yet fully defined so it would not take
 * it otherwise. It certainly would be possible to move this
 * declaration and those depending on it to avoid this problem,
 * though.
 */

/** \fn pool::pool(std::string const & name , size_t pool_size , typename worker_fifo_t::pointer_t in , typename worker_fifo_t::pointer_t out , A... args)
 * \brief Initializes a pool of worker threads.
 *
 * This constructor is used to initialize the specified number
 * of worker threads in this pool.
 *
 * At this time, all the worker threads get allocated upfront.
 *
 * \todo
 * We may want to add more threads as the work load increases
 * which could allow for much less unused threads created. To
 * do so we want to (1) time the do_work() function to get
 * an idea of how long a thread takes to perform its work;
 * and (2) have a threshold so we know when the client wants
 * to create new worker threads if the load increases instead
 * of assuming it should happen as soon as data gets added
 * to the input FIFO. The \p pool_size parameter would then
 * become a \p max_pool_size.
 *
 * \param[in] name  The name of the pool.
 * \param[in] pool_size  The number of threads to create.
 * \param[in] in  The input FIFO (where workers receive workload.)
 * \param[in] out The output FIFO (where finished work is sent.)
 * \param[in] args  Extra arguments to initialize the workers.
 */


/** \fn pool::~pool()
 * \brief Make sure that the thread pool is cleaned up.
 *
 * The destructor of the snap thread pool stops all the threads
 * and then waits on them. Assuming that your code doesn't loop
 * forever, the result is that all the threads are stopped, joined,
 * and all the incoming and outgoing data was cleared.
 *
 * If you need to get the output data, then make sure to call
 * stop(), wait(), and pop_front() until it returns false.
 * Only then can you call the destructor.
 *
 * You are safe to run that stop process in your own destructor.
 */



/** \fn pool::size() const
 * \brief Retrieve the number of workers.
 *
 * This function returns the number of workers this thread pool
 * is handling. The number is at least 1 as a thread pool can't
 * currently be empty.
 *
 * \return The number of workers in this thread pool.
 */


/** \fn pool::get_worker(int i)
 * \brief Get worker at index `i`.
 *
 * This function returns a reference to the worker at index `i`.
 *
 * \exception std::range_error
 * If the index is out of range (negative or larger or equal to
 * the number of workers) then this exception is raised.
 *
 * \param[in] i  The index of the worker to retrieve.
 *
 * \return A reference to worker `i`.
 */


/** \fn pool::get_worker(int i) const
 * \brief Get worker at index `i` (constant version).
 *
 * This function returns a reference to the worker at index `i`.
 *
 * \exception std::range_error
 * If the index is out of range (negative or larger or equal to
 * the number of workers) then this exception is raised.
 *
 * \param[in] i  The index of the worker to retrieve.
 *
 * \return A reference to worker `i`.
 */


/** \fn push_back(work_load_type const & v)
 * \brief Push one work load of data.
 *
 * This function adds a work load of data to the input. One of the
 * worker threads will automatically pick up that work and have
 * its snap_worker::do_work() function called.
 *
 * \param[in] v  The work load of data to add to this pool.
 *
 * \sa pop_front()
 */


/** \fn pop_front(work_load_type & v, int64_t usecs)
 * \brief Retrieve one work load of processed data.
 *
 * This function retrieves one T object from the output FIFO.
 *
 * The \p usecs parameter can be set to -1 to wait until output
 * is received. Set it to 0 to avoid waiting (check for data,
 * if nothing return immediately) and a positive number to wait
 * up to that many microseconds.
 *
 * Make sure to check the function return value. If false, the
 * \p v parameter is unmodified.
 *
 * Once you called the stop() function, the pop_front() function
 * can still be called until the out queue is emptied. The proper
 * sequence is to 
 *
 * \param[in] v  A reference where the object gets copied.
 * \param[in] usecs  The number of microseconds to wait.
 *
 * \return true if an object was retrieved.
 *
 * \sa snap_mutex::pop_front()
 */


/** \fn stop(bool immediate)
 * \brief Stop the threads.
 *
 * This function is called to ask all the threads to stop.
 *
 * When the \p immediate parameter is set to true, whatever is
 * left in the queue gets removed. This means you are likely to
 * get invalid or at least incomplete results in the output.
 * It should only be used if the plan is to cancel the current
 * work process and trash everything away.
 *
 * After a call to the stop() function, you may want to retrieve
 * the last bits of data with the pop_front() function until
 * it returns false and then call the join() function to wait
 * on all the threads and call pop_front() again to make sure
 * that you got all the output.
 *
 * You may also call the join() function right after stop()
 * and then the pop_front().
 *
 * \note
 * It is not necessary to call pop_front() if you are cancelling
 * the processing anyway. You can just ignore that data and
 * it will be deleted as soon as the thread pool gets deleted.
 *
 * \note
 * The function can be called any number of times. It really only
 * has an effect on the first call, though.
 *
 * \param[in] immediate  Whether the clear the remaining items on
 *                       the queue.
 */


/** \fn wait()
 * \brief Wait on the threads to be done.
 *
 * This function waits on all the worker threads until they all
 * exited. This ensures that all the output (see the pop_front()
 * function) was generated and you can therefore end your
 * processing.
 *
 * The order of processing is as follow:
 *
 * \code
 *     cppthread_pool my_pool;
 *
 *     ...initialization...
 *
 *     data_t in;
 *     data_t out;
 *
 *     while(load_data(in))
 *     {
 *         pool.push_back(in);
 *
 *         while(pool.pop_front(out))
 *         {
 *             // handle out, maybe:
 *             save_data(out);
 *         }
 *     }
 *
 *     // no more input
 *     pool.stop();
 *
 *     // optionally, empty the output pipe before the wait()
 *     while(pool.pop_front(out))
 *     {
 *         // handle out...
 *     }
 *
 *     // make sure the workers are done
 *     pool.wait();
 *
 *     // make sure the output is all managed
 *     while(pool.pop_front(out))
 *     {
 *         // handle out...
 *     }
 *
 *     // now we're done
 * \endcode
 *
 * Emptying the output queue between the stop() and wait() is not
 * required. It may always be empty or you will anyway get only
 * a few small chunks that can wait in the buffer.
 *
 * \note
 * The function can be called any number of times. After the first
 * time, though, the vector of worker threads is empty so really
 * nothing happens.
 *
 * \attention
 * This function can't be called from one of the workers.
 */


} // namespace cppthread
// vim: ts=4 sw=4 et
