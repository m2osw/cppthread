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


struct data_t
{
    int     f_value = 0;
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
    CPPTHREAD_PLUGIN_DEFAULTS(testme)

    virtual void        bootstrap(void * data);
    std::string         it_worked();

private:
};



} // optional_namespace namespace
// vim: ts=4 sw=4 et
