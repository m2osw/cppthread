// Copyright (c) 2006-2021  Made to Order Software Corp.  All Rights Reserved
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

// self
//
#include    "main.h"


// cppthread lib
//
#include    <cppthread/exception.h>
#include    <cppthread/fifo.h>
#include    <cppthread/guard.h>
#include    <cppthread/life.h>
#include    <cppthread/mutex.h>
#include    <cppthread/pool.h>
#include    <cppthread/runner.h>
#include    <cppthread/thread.h>
#include    <cppthread/worker.h>


// snapdev lib
//
#include    <snapdev/not_reached.h>


// C lib
//
#include    <unistd.h>



CATCH_TEST_CASE("example", "[example]")
{
    CATCH_START_SECTION("Simple threading")
    {
        // TODO...
    }
    CATCH_END_SECTION()
}


// vim: ts=4 sw=4 et
