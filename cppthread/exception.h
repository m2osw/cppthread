// Copyright (c) 2013-2021  Made to Order Software Corp.  All Rights Reserved
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
 * \brief Exceptions for the thread environment.
 *
 * This file includes the definitions of exceptions used by the C++ Thread
 * environment.
 */

// libexcept lib
//
#include    <libexcept/exception.h>


namespace cppthread
{



DECLARE_LOGIC_ERROR(cppthread_logic_error);

DECLARE_OUT_OF_RANGE(cppthread_out_of_range);

DECLARE_MAIN_EXCEPTION(cppthread_exception);
DECLARE_EXCEPTION(cppthread_exception, cppthread_already_exists);
DECLARE_EXCEPTION(cppthread_exception, cppthread_in_use_error);
DECLARE_EXCEPTION(cppthread_exception, cppthread_invalid_error);
DECLARE_EXCEPTION(cppthread_exception, cppthread_invalid_log_level);
DECLARE_EXCEPTION(cppthread_exception, cppthread_mutex_failed_error);
DECLARE_EXCEPTION(cppthread_exception, cppthread_name_mismatch);
DECLARE_EXCEPTION(cppthread_exception, cppthread_not_found);
DECLARE_EXCEPTION(cppthread_exception, cppthread_not_locked_error);
DECLARE_EXCEPTION(cppthread_exception, cppthread_not_locked_once_error);
DECLARE_EXCEPTION(cppthread_exception, cppthread_not_started);
DECLARE_EXCEPTION(cppthread_exception, cppthread_system_error);



} // namespace cppthread
// vim: ts=4 sw=4 et
