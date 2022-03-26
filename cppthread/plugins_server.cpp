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

// self
//
#include    "cppthread/plugins_server.h"

//#include    "cppthread/plugins_definition.h"

// #include    "cppthread/log.h"
// #include    "cppthread/guard.h"
#include    "cppthread/version.h"


// // snapdev
// //
// #include    <snapdev/glob_to_list.h>
// #include    <snapdev/join_strings.h>
// #include    <snapdev/not_used.h>
// #include    <snapdev/tokenize_string.h>
// 
// 
// // C
// //
// #include    <dlfcn.h>

// last include
//
#include    <snapdev/poison.h>




namespace cppthread
{


namespace detail
{



::cppthread::plugin_definition const g_plugin_server_definition = ::cppthread::define_plugin(
      ::cppthread::plugin_version(::cppthread::version_t(1, 0, 0))
    , ::cppthread::plugin_library_version(::cppthread::version_t(CPPTHREAD_VERSION_MAJOR, CPPTHREAD_VERSION_MINOR, CPPTHREAD_VERSION_PATCH))
    , ::cppthread::plugin_last_modification(UTC_BUILD_TIME_STAMP)
    , ::cppthread::plugin_name("server")
);

plugin_server_factory::plugin_server_factory(server::pointer_t s)
    : plugin_factory(g_plugin_server_definition, std::dynamic_pointer_cast<plugin>(s))
{
    save_factory_in_plugin(s.get());
    register_plugin("server", instance());
}


plugin_server_factory * g_plugin_server_factory = nullptr;




} // namespace detail


server::server()
{
}

server::~server()
{
}

server::pointer_t server::instance()
{
    return std::static_pointer_cast<server>(detail::g_plugin_server_factory->instance());
}



} // namespace cppthread
// vim: ts=4 sw=4 et
