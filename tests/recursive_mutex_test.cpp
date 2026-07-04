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

// C++
//
#include    <atomic>
#include    <stdexcept>
#include    <iostream>


// C
//
#include    <pthread.h>



pthread_mutex_t     g_mutex = pthread_mutex_t();
pthread_cond_t      g_condition = pthread_cond_t();
pthread_t           g_thread_id[3] = {};
std::atomic<bool>   g_done = false;


void init_mutex()
{
    pthread_mutexattr_t mattr;
    int err(pthread_mutexattr_init(&mattr));
    if(err != 0)
    {
        throw std::runtime_error("could not initialize mutex attributes.");
    }
    err = pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE);
    if(err != 0)
    {
        throw std::runtime_error("could not setup mutex type to recursive.");
    }
    err = pthread_mutex_init(&g_mutex, &mattr);
    if(err != 0)
    {
        throw std::runtime_error("could not initialize mutex.");
    }
    err = pthread_mutexattr_destroy(&mattr);
    if(err != 0)
    {
        throw std::runtime_error("could not destroy mutex attributes.");
    }
}


void init_condition()
{
    pthread_condattr_t cattr;
    int err(pthread_condattr_init(&cattr));
    if(err != 0)
    {
        throw std::runtime_error("could not initialize condition attributes.");
    }
    err = pthread_cond_init(&g_condition, &cattr);
    if(err != 0)
    {
        throw std::runtime_error("could not initialize condition.");
    }
    err = pthread_condattr_destroy(&cattr);
    if(err != 0)
    {
        throw std::runtime_error("could not destroy condition attributes.");
    }
}


void lock()
{
    int const err(pthread_mutex_lock(&g_mutex));
    if(err != 0)
    {
        throw std::runtime_error("could not lock mutex.");
    }
}


void unlock()
{
    int const err(pthread_mutex_unlock(&g_mutex));
    if(err != 0)
    {
        throw std::runtime_error("could not unlock mutex.");
    }
}


void wait()
{
    int const err(pthread_cond_wait(&g_condition, &g_mutex));
    if(err != 0)
    {
        throw std::runtime_error("could not wait for condition.");
    }
}


void * thread_one(void *)
{
    lock();
    std::cout << "thread one started!" << std::endl;
    unlock();

#ifdef EXTRA_LOCK
    lock();
#endif

    while(!g_done)
    {
        lock();
        wait();
        unlock();

        lock();
        std::cout << "thread one wait() returned!" << std::endl;
        unlock();
    }

#ifdef EXTRA_LOCK
    unlock();
#endif

    return nullptr;
}


void * thread_two(void *)
{
    lock();
    std::cout << "thread two started!" << std::endl;
    unlock();

    while(!g_done)
    {
        lock();
        wait();
        unlock();

        lock();
        std::cout << "thread two wait() returned!" << std::endl;
        unlock();
    }

    return nullptr;
}


void * thread_three(void *)
{
    lock();
    std::cout << "thread three started!" << std::endl;
    unlock();

    while(!g_done)
    {
        lock();
        wait();
        unlock();

        lock();
        std::cout << "thread three wait() returned!" << std::endl;
        unlock();
    }

    return nullptr;
}


void start_thread(int index, void * (*runner)(void *))
{
    pthread_attr_t tattr;
    int err(pthread_attr_init(&tattr));
    if(err != 0)
    {
        throw std::runtime_error("could not initialize thread attributes.");
    }
    err = pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_JOINABLE);
    if(err != 0)
    {
        throw std::runtime_error("could not set thread attribute to joinable.");
    }
    err = pthread_create(&g_thread_id[index], &tattr, runner, nullptr);
    if(err != 0)
    {
        pthread_attr_destroy(&tattr);
        throw std::runtime_error("could not create thread.");
    }
    err = pthread_attr_destroy(&tattr);
    if(err != 0)
    {
        throw std::runtime_error("could not destroy thread attributes.");
    }
}


void signal_one_thread()
{
    lock();
    int const err(pthread_cond_signal(&g_condition));
    unlock();
    if(err != 0)
    {
        throw std::runtime_error("broadcasting signal failed.");
    }
}


void signal_end()
{
    g_done = true;

    int const err(pthread_cond_broadcast(&g_condition));
    if(err != 0)
    {
        throw std::runtime_error("broadcasting signal failed.");
    }
}


void stop_thread(int index)
{
    int err(pthread_join(g_thread_id[index], nullptr));
    if(err != 0)
    {
        throw std::runtime_error("could not join with thread.");
    }
}


int main(int, char *[])
{
    try
    {
        std::cout << "--- recursive mutex test started ---" << std::endl;

        std::cout << "--- initialize mutex ---" << std::endl;
        init_mutex();
        std::cout << "--- initialize condition ---" << std::endl;
        init_condition();

        std::cout << "--- start threads ---" << std::endl;
        start_thread(0, &thread_one);
        start_thread(1, &thread_two);
        start_thread(2, &thread_three);

        std::cout << "--- pause ---" << std::endl;
        sleep(1);

        std::cout << "--- 10 signals ---" << std::endl;
        for(int count(0); count < 10; ++count)
        {
            signal_one_thread();
            sleep(1);
        }

        std::cout << "--- signal end ---" << std::endl;
        signal_end();

        std::cout << "--- stop threads ---" << std::endl;
        stop_thread(0);
        stop_thread(1);
        stop_thread(2);

        std::cout << "--- recursive mutex test ended ---" << std::endl;
    }
    catch(std::runtime_error const & e)
    {
        std::cout << "--- abort with exception: "
            << e.what()
            << " ---" << std::endl;
    }

    return 0;
}


// vim: ts=4 sw=4 et

