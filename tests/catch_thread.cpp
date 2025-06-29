// Copyright (c) 2006-2025  Made to Order Software Corp.  All Rights Reserved
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

// cppthread
//
#include    <cppthread/exception.h>
#include    <cppthread/fifo.h>
#include    <cppthread/guard.h>
#include    <cppthread/life.h>
#include    <cppthread/log.h>
#include    <cppthread/mutex.h>
#include    <cppthread/pool.h>
#include    <cppthread/runner.h>
#include    <cppthread/thread.h>
#include    <cppthread/worker.h>


// self
//
#include    "catch_main.h"


// snapdev
//
#include    <snapdev/not_reached.h>


// C++
//
#include    <deque>



namespace
{


struct log_message_t
{
    typedef std::deque<log_message_t>   queue_t;

    cppthread::log_level_t  f_level = static_cast<cppthread::log_level_t>(-1);      // an invalid level by default
    std::string             f_message = std::string();
};


log_message_t::queue_t g_message_queue = log_message_t::queue_t();
int g_empty_log_queue = 0;
int g_unexpected_log_message = 0;
int g_unexpected_log_level = 0;


void cppthread_log_callback(cppthread::log_level_t level, std::string const & message)
{
    cppthread::guard lock(*cppthread::g_system_mutex);

    if(g_message_queue.empty())
    {
        ++g_empty_log_queue;
    }
    else
    {
        if(g_message_queue.front().f_level != level)
        {
            ++g_unexpected_log_level;
        }
        if(g_message_queue.front().f_message != message)
        {
            ++g_unexpected_log_message;
        }
        g_message_queue.pop_front();
    }
}


constexpr int EXIT_THREAD = -1;
constexpr int EXIT_THREAD_WITH_EXCEPTION = -2;


DECLARE_EXCEPTION(cppthread::cppthread_exception, exit_with_exception);


struct data_t
{
    typedef std::deque<data_t>      queue_t;

    int     f_value = 0;

    bool operator == (data_t const & rhs) const
    {
        return f_value == rhs.f_value;
    }
};


typedef cppthread::fifo<data_t>     fifo_t;


class test_runner
    : public cppthread::runner
{
public:
    test_runner()
        : runner("test-runner")
    {
    }

    virtual bool is_ready() const override
    {
        return f_ready;
    }

    virtual bool continue_running() const override
    {
        if(!runner::continue_running())
        {
            return false;
        }

        return f_continue_running;
    }

    virtual void enter() override
    {
        f_entered = true;
    }

    virtual void run() override
    {
        while(continue_running())
        {
            data_t data;
            if(!f_fifo.pop_front(data, 500))
            {
                ++f_cycles;
                continue;
            }
            if(f_expected.empty())
            {
                ++f_unexpected_content;
            }
            else
            {
//std::cerr << "--- got data: " << data.f_value << " (expects: " << f_expected.front().f_value << ")\n";
                if(data != f_expected.front())
                {
                    ++f_invalid_content;
                }
                else
                {
                    switch(data.f_value)
                    {
                    case EXIT_THREAD:
                        return;

                    case EXIT_THREAD_WITH_EXCEPTION:
                        throw exit_with_exception("testing thread exiting with exception.");

                    }
                }
                f_expected.pop_front();
            }
        }

        f_stopped_running = true;
    }

    virtual void leave(cppthread::leave_status_t status) override
    {
        f_leave_status = status;
    }

    fifo_t                      f_fifo = fifo_t();
    data_t::queue_t             f_expected = data_t::queue_t();
    bool                        f_ready = true;
    bool                        f_continue_running = true;
    bool                        f_entered = false;
    bool                        f_stopped_running = false; // if true, the continue_running() function returned false
    int                         f_cycles = 0;
    int                         f_unexpected_content = 0;
    int                         f_invalid_content = 0;
    cppthread::leave_status_t   f_leave_status = cppthread::leave_status_t();
};


cppthread::thread * g_stop_callback_thread = nullptr;

void stop_callback(cppthread::thread * t)
{
    g_stop_callback_thread = t;
}



} // no name namespace


CATCH_TEST_CASE("cppthread", "[thread][valid]")
{
    CATCH_START_SECTION("cppthread: simple threading")
    {
        cppthread::set_log_callback(cppthread_log_callback);

        test_runner r;
        cppthread::thread t("test-thread", &r);
        CATCH_REQUIRE(t.get_runner() == &r);
        CATCH_REQUIRE(t.get_name() == "test-thread");
        CATCH_REQUIRE_FALSE(t.is_running());

        // first check that the is_ready() works as expected
        //
        r.f_ready = false;
        g_message_queue.push_back({
                cppthread::log_level_t::warning,
                "the thread runner is not ready.",
            });
        CATCH_REQUIRE_FALSE(t.start());
        CATCH_REQUIRE(g_message_queue.empty());
        CATCH_REQUIRE_FALSE(t.is_running());
        r.f_ready = true;

        // WARNING: at the moment this queue is not protected so create it
        //          at the start and then check once the thread is done that
        //          it is indeed empty
        //
        r.f_expected.push_back({1});
        r.f_expected.push_back({2});
        r.f_expected.push_back({3});
        r.f_expected.push_back({EXIT_THREAD});

        // now really start the runner
        //
        CATCH_REQUIRE(t.start());
        CATCH_REQUIRE(t.is_running());

        // now another attempt at calling start() fails with a log message
        //
        g_message_queue.push_back({
                cppthread::log_level_t::warning,
                "the thread is already running.",
            });
        CATCH_REQUIRE_FALSE(t.start());
        CATCH_REQUIRE(g_message_queue.empty());

        timespec wait = {0, 100'000'000};
        timespec rem = timespec();

        nanosleep(&wait, &rem);
        r.f_fifo.push_back({1});
        nanosleep(&wait, &rem);
        r.f_fifo.push_back({2});
        nanosleep(&wait, &rem);
        r.f_fifo.push_back({3});
        nanosleep(&wait, &rem);
        CATCH_REQUIRE(t.is_running()); // after this point, we pushed the EXIT_THREAD so the thread may quit any time now
        r.f_fifo.push_back({EXIT_THREAD});
        nanosleep(&wait, &rem);

        // wait for the runner to stop (join with the thread)
        //
        t.stop(&stop_callback);
        CATCH_REQUIRE(g_stop_callback_thread == &t);
        CATCH_REQUIRE_FALSE(t.is_running());

        CATCH_REQUIRE(r.f_cycles > 0);
        CATCH_REQUIRE_FALSE(r.f_stopped_running);

    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("cppthread: create runner and throw an exception")
    {
        cppthread::set_log_callback(cppthread_log_callback);

        test_runner r;
        cppthread::thread t("test-thread", &r);

        bool got_exception(false);
        try
        {
            // WARNING: at the moment this queue is not protected so create it
            //          at the start and then check once the thread is done that
            //          it is indeed empty
            //
            r.f_expected.push_back({1});
            r.f_expected.push_back({2});
            r.f_expected.push_back({3});
            r.f_expected.push_back({EXIT_THREAD_WITH_EXCEPTION});

            // now really start the runner
            //
            CATCH_REQUIRE(t.start());

            timespec wait = {0, 100'000'000};
            timespec rem = timespec();

            nanosleep(&wait, &rem);
            r.f_fifo.push_back({1});
            nanosleep(&wait, &rem);
            r.f_fifo.push_back({2});
            nanosleep(&wait, &rem);
            r.f_fifo.push_back({3});
            nanosleep(&wait, &rem);
            r.f_fifo.push_back({EXIT_THREAD_WITH_EXCEPTION});
            //wait.tv_nsec = 500'000'000;
            nanosleep(&wait, &rem);

            // now stop the thread
            //
            t.stop();
        }
        catch(exit_with_exception const & e)
        {
            CATCH_REQUIRE(std::string(e.what()) == "cppthread_exception: testing thread exiting with exception.");
            got_exception = true;
        }

        CATCH_REQUIRE(got_exception);
        CATCH_REQUIRE(t.get_exception() == std::exception_ptr()); // the stop() clears the exception in the thread
        CATCH_REQUIRE(r.f_cycles > 0);
        CATCH_REQUIRE_FALSE(r.f_stopped_running);

        CATCH_REQUIRE(g_message_queue.empty());
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("cppthread: create runner and throw an exception, but auto-destruction drops the exception")
    {
        cppthread::set_log_callback(cppthread_log_callback);

        test_runner r;

        bool got_exception(false);
        try
        {
            cppthread::thread t("test-thread", &r);

            // WARNING: at the moment this queue is not protected so create it
            //          at the start and then check once the thread is done that
            //          it is indeed empty
            //
            r.f_expected.push_back({1});
            r.f_expected.push_back({2});
            r.f_expected.push_back({3});
            r.f_expected.push_back({EXIT_THREAD_WITH_EXCEPTION});

            // now really start the runner
            //
            CATCH_REQUIRE(t.start());

            timespec wait = {0, 100'000'000};
            timespec rem = timespec();

            nanosleep(&wait, &rem);
            r.f_fifo.push_back({1});
            nanosleep(&wait, &rem);
            r.f_fifo.push_back({2});
            nanosleep(&wait, &rem);
            r.f_fifo.push_back({3});
            nanosleep(&wait, &rem);
            r.f_fifo.push_back({EXIT_THREAD_WITH_EXCEPTION});
            //wait.tv_nsec = 500'000'000;
            nanosleep(&wait, &rem);

            // delete the thread using RAII, we lose the exception...
            // (the destructor cannot re-throw, only std::terminate()...)
        }
        catch(exit_with_exception const & e)
        {
            CATCH_REQUIRE(std::string(e.what()) == "cppthread_exception: testing thread exiting with exception.");
            got_exception = true;
        }

        CATCH_REQUIRE_FALSE(got_exception);
        CATCH_REQUIRE(r.f_cycles > 0);
        CATCH_REQUIRE_FALSE(r.f_stopped_running);

        CATCH_REQUIRE(g_message_queue.empty());
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("cppthread_errors", "[thread][invalid]")
{
    CATCH_START_SECTION("cppthread: runner cannot be a null pointer")
    {
        CATCH_REQUIRE_THROWS_MATCHES(
                  cppthread::thread("test-thread", nullptr)
                , cppthread::invalid_error
                , Catch::Matchers::ExceptionMessage("cppthread_exception: runner missing in thread() constructor."));

        // null smart pointer has the same effect
        //
        cppthread::runner::pointer_t r;
        CATCH_REQUIRE_THROWS_MATCHES(
                  cppthread::thread("test-thread", r)
                , cppthread::invalid_error
                , Catch::Matchers::ExceptionMessage("cppthread_exception: runner missing in thread() constructor."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("cppthread: one runner cannot simultaneously be used with multiple threads")
    {
        test_runner r;
        cppthread::thread t("okay", &r);

        CATCH_REQUIRE_THROWS_MATCHES(
                  cppthread::thread("breaks", &r)
                , cppthread::in_use_error
                , Catch::Matchers::ExceptionMessage("cppthread_exception: this runner (test-runner) is already in use."));
    }
    CATCH_END_SECTION()
}



// vim: ts=4 sw=4 et
