// Copyright (c) 2006-2022  Made to Order Software Corp.  All Rights Reserved
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

// cppthread lib
//
#include    <cppthread/plugins.h>



namespace optional_namespace
{


// TODO: This is generally in a separate header file, not along the
//       plugin(s); for the test at this point it's the same file;
//       look at create a new daemon.h file instead
//
class daemon
    : public cppthread::server
{
public:
    // in most cases our daemons are given the argc/argv parameters from
    // main() and the daemon parses those with advgetopt
    //
    daemon(int argc, char * argv[]);

    // we already have the plugin defaults in the cppthread::server
    //CPPTHREAD_PLUGIN_DEFAULTS(daemon);
    typedef std::shared_ptr<daemon>     pointer_t;

    int f_value = 0xA987;
};


/** \brief A test plugin class.
 *
 * All plugins must be derived from cppthread::plugin and include the
 * CPPTHREAD_PLUGIN_DEFAULTS() macro to generate the default header data.
 */
class testme
    : public cppthread::plugin
{
public:
    CPPTHREAD_PLUGIN_DEFAULTS(testme);

    virtual void        bootstrap();
    virtual std::string it_worked();

private:
};



} // optional_namespace namespace
// vim: ts=4 sw=4 et
