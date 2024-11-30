// Copyright (c) 2013-2024  Made to Order Software Corp.  All Rights Reserved
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
#pragma once

/** \file
 * \brief Thread Runner and Managers.
 *
 * This file includes the declaration and implementation (For templates)
 * of classes used to manage threads the easy way. Especially, our
 * implementation is aware of object destructors so a thread manager
 * (snap_thread) can be destroyed. It will automatically and properly
 * wait for its runner (the actual system pthread) to exit before
 * finishing up its and its runner clean up.
 */

// self
//
#include    <cppthread/mutex.h>



// libexcept
//
#include    <libexcept/scoped_signal_mask.h>


// C++
//
#include    <functional>
#include    <string>
#include    <vector>



namespace cppthread
{


enum class leave_status_t;
class runner;


constexpr pid_t                 PID_UNDEFINED = static_cast<pid_t>(-1);
constexpr pthread_t             THREAD_UNDEFINED = static_cast<pthread_t>(-1);


class thread
{
public:
    typedef std::shared_ptr<thread>     pointer_t;
    typedef std::vector<pointer_t>      vector_t;

                                thread(std::string const & name, runner * runner);
                                thread(std::string const & name, std::shared_ptr<runner> runner);
                                thread(thread const & rhs) = delete;
                                ~thread();

    thread &                    operator = (thread const & rhs) = delete;

    std::string const &         get_name() const;
    runner *                    get_runner() const;
    bool                        is_running() const;
    bool                        is_stopping() const;
    bool                        start();
    void                        stop(std::function<void(thread *)> callback = nullptr);
    bool                        kill(int sig);
    void                        mask_signals(libexcept::sig_list_t list);
    void                        mask_all_signals();
    void                        unmask_signals(libexcept::sig_list_t list);
    pid_t                       get_thread_tid() const;
    mutex &                     get_thread_mutex() const;
    void                        set_log_all_exceptions(bool log_all_exceptions);
    bool                        get_log_all_exceptions() const;
    std::exception_ptr          get_exception() const;

private:
    void                        init();

    // internal function to start the runner
    friend void *               func_internal_start(void * system_thread);
    void                        internal_thread();
    bool                        internal_enter();
    bool                        internal_run();
    void                        internal_leave(leave_status_t status);

    std::string const           f_name = std::string();
    runner *                    f_runner = nullptr;
    mutable mutex               f_mutex = mutex();
    bool                        f_running = false;
    bool                        f_started = false;
    bool                        f_stopping = false;
    bool                        f_log_all_exceptions = true;
    pid_t                       f_tid = PID_UNDEFINED;
    pthread_t                   f_thread_id = THREAD_UNDEFINED;
    pthread_attr_t              f_thread_attr = pthread_attr_t();
    std::exception_ptr          f_exception = std::exception_ptr();
};


int                 get_total_number_of_processors();
int                 get_number_of_available_processors();
pid_t               gettid();
pid_t               get_pid_max();
int                 set_current_thread_name(std::string const & name);
int                 set_thread_name(pid_t tid, std::string const & name);
std::string         get_current_thread_name();
std::string         get_thread_name(pid_t tid);

typedef std::vector<pid_t>      process_ids_t;

process_ids_t       get_thread_ids(pid_t pid = -1);
bool                is_process_running(pid_t pid);
std::string         get_boot_id();
std::size_t         get_thread_count();
bool                is_using_vdso();


} // namespace cppthread
// vim: ts=4 sw=4 et
