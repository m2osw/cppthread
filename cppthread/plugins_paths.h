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

// // self
// //
// #include    "cppthread/mutex.h"
// 
// #include    "cppthread/exception.h"
// #include    "cppthread/version.h"       // used in CPPTHREAD_PLUGIN_START()
// 
// 
// // snapdev lib
// //
// #include    <snapdev/not_used.h>


// C++ set
//
// #include    <map>
// #include    <memory>
// #include    <set>
#include    <string>
#include    <vector>


namespace cppthread
{



class plugin_paths
{
public:
    typedef std::string                 path_t;
    typedef std::vector<std::string>    paths_t;

    std::size_t                         size() const;
    std::string                         at(std::size_t idx) const;
    void                                set_allow_redirects(bool allow = true);
    bool                                get_allow_redirects() const;
    path_t                              canonicalize(path_t const & path);
    void                                push(path_t const & path);
    void                                add(std::string const & paths);
    void                                erase(std::string const & path);

private:
    paths_t                             f_paths = paths_t();
    bool                                f_allow_redirects = false;
};



} // namespace cppthread
// vim: ts=4 sw=4 et
