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
#include    "cppthread/plugins_names.h"
#include    "cppthread/plugins_server.h"

#include    "cppthread/mutex.h"
// #include    "cppthread/exception.h"
#include    "cppthread/version.h"       // used in CPPTHREAD_PLUGIN_START()


// // snapdev lib
// //
// #include    <snapdev/not_used.h>


// C++ set
//
// #include    <map>
#include    <memory>
// #include    <set>
// #include    <string>
// #include    <vector>


namespace cppthread
{



class plugin_collection
{
public:
    typedef std::shared_ptr<plugin_collection>  pointer_t;

                                        plugin_collection(plugin_names const & names);
                                        plugin_collection(plugin_collection const &) = delete;
    plugin_collection &                 operator = (plugin_collection const &) = delete;

    bool                                load_plugins(server::pointer_t s);
    bool                                is_loaded(std::string const & name) const;

    template<typename T>
    typename T::pointer_t               get_server()
    {
        return std::dynamic_pointer_cast<T>(f_server);
    }

    /** \brief Retrieve a plugin from this collection.
     *
     * This function searches for a plugin by the given name in this collection.
     *
     * Note that different collections can share the same plugin (if the filenames
     * are the same) and one collection may know about a plugin and another
     * collection may not know about a plugin. At this time, the library does not
     * offer a direct access to the global list so you can't determine whether
     * a specific plugin is loaded through a plugin_collection.
     *
     * \param[in] name  The name of the plugin to search.
     *
     * \return The pointer to the plugin if found, nullptr otherwise.
     */
    template<typename T>
    typename T::pointer_t               get_plugin_by_name(std::string const & name)
    {
        auto it(f_plugins_by_name.find(name));
        if(it != f_plugins_by_name.end())
        {
            return std::static_pointer_cast<T>(it->second);
        }

        return typename T::pointer_t();
    }

    // TBD: I think I prefer to use the get_server() rather than the data pointer
    //      but in the Snap! C++ plugins we have a server and a a snap_child...
    //
    template<typename T>
    T *                                 get_data() const { return static_cast<T *>(f_data); }
    void                                set_data(void * data);

private:
    mutex                               f_mutex = mutex();
    plugin_names                        f_names;
    plugin::map_t                       f_plugins_by_name = plugin::map_t();        // plugins sorted by name only
    plugin::vector_t                    f_ordered_plugins = plugin::vector_t();     // sorted plugins
    void *                              f_data = nullptr;
    server::pointer_t                   f_server = server::pointer_t();
};



} // namespace cppthread
// vim: ts=4 sw=4 et
