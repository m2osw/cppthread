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
 * \brief Implementation of the Thread Life class.
 *
 * This file includes the implementation of the life class. It allows
 * you to manage the lifetime of a thread base on your stack or another
 * object.
 */


// self
//
#include    "cppthread/life.h"

#include    "cppthread/exception.h"
#include    "cppthread/thread.h"


// last include
//
#include    "snapdev/poison.h"




namespace cppthread
{



/** \class life
 * \brief An RAII class managing the lifetime of a thread.
 *
 * This class is used to manage the life of a thread: the time it runs.
 * The constructor calls the thread::start() function and
 * the destructor makes sure to call the thread::stop() function.
 *
 * If you have a specific block or another class that should run a
 * thread for the lifetime of the block or class object, then this
 * is well adapted.
 *
 * \note
 * This class is not responsible for deleting the thread at the
 * end. It only manages the time while the thread runs.
 */


/** \brief Initialize a "thread life" object.
 *
 * This type of objects are used to record a thread and make sure
 * that it gets destroyed once done with it.
 *
 * The constructor makes sure that the specified thread is not
 * a null pointer and it starts the thread. If the thread is
 * already running, then the constructor will throw.
 *
 * Once such an object was created, it is not possible to prevent
 * the thread life destructor from calling the stop() function and
 * waiting for the thread to be done.
 *
 * \note
 * If possible, you should consider using the thread::pointer_t
 * instead of the life which expects a bare pointer.
 * There are situations, though, where this class is practical
 * because you have a thread as a variable member in a class.
 *
 * \param[in] thread  The thread which life is to be controlled.
 */
life::life(thread * const thread)
    : f_thread(thread)
{
    if(f_thread == nullptr)
    {
        throw cppthread_logic_error("life pointer is nullptr");
    }
    if(!f_thread->start())
    {
        // we cannot really just generate an error if the thread
        // does not start because we do not offer a way for the
        // user to know so we have to throw for now
        throw cppthread_not_started("somehow the thread was not started, an error should have been logged");
    }
}


/** \brief Make sure the thread stops.
 *
 * This function requests that the attach thread stop. It will block
 * until such happens. You are responsible to make sure that the
 * stop happens early on if your own object needs to access the
 * thread while stopping.
 */
life::~life()
{
    //log << log_level_t::debug
    //    << "stopping life..."
    //    << end;

    f_thread->stop();

    //log << log_level_t::debug
    //    << "life stopped!"
    //    << end;
}



/** \fn life::life(life const & rhs)
 * \brief The copy operator is deleted.
 *
 * The life object holds a bare pointer to the thread it has to manage,
 * so we have to declare a copy operator to explicitly delete the copy
 * operator. We could not have multiple instances of the life object
 * anyway.
 *
 * \param[in] rhs  The right hand side.
 */


/** \fn life::operator = (life const & rhs)
 * \brief The assignment operator is deleted.
 *
 * The life object holds a bare pointer to the thread it has to manage
 * so we have to declare an assignment operator to explicitly delete
 * the operator. We could not have multiple instances of the life object
 * anyway.
 *
 * \param[in] rhs  The right hand side.
 *
 * \return A reference to this object.
 */


/** \var life::f_thread
 * \brief The pointer to the thread being managed.
 *
 * This pointer is a pointer to the thread. Once a thread life
 * object is initialized, the pointer is never nullptr (we
 * throw before in the constructor if that is the case.)
 *
 * The user of the life class must view the
 * thread pointer as owned by the life object
 * (similar to a smart pointer.)
 */




} // namespace cppthread
// vim: ts=4 sw=4 et
