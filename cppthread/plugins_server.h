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
#pragma once

// self
//
#include    "cppthread/plugins_factory.h"
#include    "cppthread/plugins.h"

// #include    "cppthread/exception.h"
// #include    "cppthread/version.h"       // used in CPPTHREAD_PLUGIN_START()
// 
// 
// // snapdev lib
// //
// #include    <snapdev/not_used.h>
// 
// 
// // C++ set
// //
// #include    <map>
// #include    <memory>
// #include    <set>
// #include    <string>
// #include    <vector>


namespace cppthread
{



class server
    : public plugin
{
public:
    // Do not use the CPPTHREAD_PLUGIN_DEFAULTS() because at this point
    // we do not have access to the factory
    //
    typedef std::shared_ptr<server> pointer_t;

                            server();
                            server(server const &) = delete;
    virtual                 ~server();
    server &                operator = (server const &) = delete;
    static pointer_t        instance();
};



namespace detail
{

class plugin_server_factory
    : public plugin_factory
{
public:
    plugin_server_factory(server::pointer_t s);
    plugin_server_factory(plugin_server_factory const &) = delete;
    plugin_server_factory & operator = (plugin_server_factory const &) = delete;
};

extern plugin_server_factory * g_plugin_server_factory;

} // namespace detail



} // namespace cppthread
// vim: ts=4 sw=4 et
