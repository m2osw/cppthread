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
#include    "cppthread/plugins_paths.h"

// #include    "cppthread/exception.h"
// #include    "cppthread/version.h"       // used in CPPTHREAD_PLUGIN_START()
// 
// 
// // snapdev lib
// //
// #include    <snapdev/not_used.h>


// C++ set
//
#include    <map>
// #include    <memory>
// #include    <set>
// #include    <string>
// #include    <vector>


namespace cppthread
{



class plugin_names
{
public:
    typedef std::string                     name_t;
    typedef std::string                     filename_t;
    typedef std::map<name_t, filename_t>    names_t;

                                        plugin_names(plugin_paths const & paths, bool script_names = false);

    bool                                validate(name_t const & name);
    bool                                is_emcascript_reserved(std::string const & word);

    filename_t                          to_filename(name_t const & name);
    void                                push(name_t const & name);
    void                                add(std::string const & set);
    names_t                             names() const;

    void                                find_plugins(name_t const & prefix = name_t(), name_t const & suffix = name_t());

private:
    plugin_paths const                  f_paths;
    bool const                          f_prevent_script_names = false;
    names_t                             f_names = names_t();
};



} // namespace cppthread
// vim: ts=4 sw=4 et
