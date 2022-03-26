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
#include    "cppthread/plugins_repository.h"

#include    "cppthread/log.h"
#include    "cppthread/guard.h"


// // snapdev
// //
// #include    <snapdev/glob_to_list.h>
// #include    <snapdev/join_strings.h>
// #include    <snapdev/not_used.h>
// #include    <snapdev/tokenize_string.h>


// C
//
#include    <dlfcn.h>

// last include
//
#include    <snapdev/poison.h>



namespace cppthread
{
namespace detail
{



/** \class plugin_repository
 * \brief The global Plugin Repository.
 *
 * A plugin is always considered global, as far as the dlopen() function is
 * concerned, loading the exact same .so multiple times has no effect the
 * second, third, etc. time (it just increments a reference counter).
 *
 * So on our end we have to have a global table of plugins. If the same plugin
 * is to be loaded
 *
 * \note
 * The plugin_repository is a singleton.
 */



/** \brief Retrieve an instance of the plugin repository.
 *
 * Each plugin can be loaded only once, but it can be referenced in multiple
 * plugin_collection. So what we do is use a singleton, the plugin_repository,
 * which does the actual load of the plugin through the get_plugin(). If
 * the plugin is already loaded, then it get returned immediately. If not
 * there, then we use the dlopen() function to load it. At that point, the
 * plugin itself will register itself.
 *
 * This function returns a pointer to the singleton so we can access the
 * get_plugin() to retrieve a plugin and the register_plugin() from the
 * factory to register the plugin in this singleton.
 *
 * \return The plugin_repository reference.
 */
plugin_repository & plugin_repository::instance()
{
    static mutex g_mutex;

    guard lock(g_mutex);

    static plugin_repository * g_plugin_repository = nullptr;

    if(g_plugin_repository == nullptr)
    {
        g_plugin_repository = new plugin_repository;
    }

    return *g_plugin_repository;
}


/** \brief Get a pointer to the specified plugin.
 *
 * This function returns a pointer to the plugin found in \p filename.
 *
 * If the dlopen() function fails, then the function returns a null pointer
 * in which case the function generates an error in the log. The two main
 * reasons for the load to fail are: (1) the file is not a valid plugin
 * and (2) the plugin has unsatisfied link dependencies, however, on that
 * second point, the linker is used lazily so in most cases you detect
 * those errors later when you call functions in your plugins.
 *
 * \note
 * This function, by itself, is expected to be thread safe although it is
 * suggested that you consider loading your plugins before creating threads.
 *
 * \param[in] filename  The name of the file that corresponds to a plugin.
 *
 * \return The pointer to the plugin.
 */
plugin::pointer_t plugin_repository::get_plugin(plugin_names::filename_t const & filename)
{
    guard lock(f_mutex);

    // first check whether it was already loaded, if so, just return the
    // existing plugin (no need to re-load it)
    //
    auto it(f_plugins.find(filename));
    if(it != f_plugins.end())
    {
        return it->second;
    }

    // TBD: Use RTLD_NOW instead of RTLD_LAZY in DEBUG mode
    //      so we discover missing symbols would be nice, only
    //      that would require loading in the correct order...
    //      (see dlopen() call below)
    //

    // load the plugin; the plugin will "register itself" through its factory
    //
    // we want to plugin filename save in the plugin object itself at the time
    // we register it so we save it here and pick it up at the time the
    // registration function gets called
    //
    f_register_filename = filename;
    void const * const h(dlopen(filename.c_str(), RTLD_LAZY | RTLD_GLOBAL));
    if(h == nullptr)
    {
        int const e(errno);
        log << log_level_t::error
            << "cannot load plugin file \""
            << filename
            << "\" (errno: "
            << e
            << ", "
            << dlerror()
            << ")"
            << end;
        return plugin::pointer_t();
    }
    f_register_filename.clear();

    log << log_level_t::debug
        << "loaded plugin: \""
        << filename
        << "\""
        << end;

    return f_plugins[filename];
}


/** \brief Register the plugin in our global repository.
 *
 * Since plugins can be loaded once with dlopen() and then reused any number
 * of times, we register the plugins in a global repositiory. This function
 * is the one used to actually register the plugin.
 *
 * When loading a new plugin you should call the
 * plugin_repository::get_plugin() with the plugin filename. If that plugin
 * was already loaded, then that pointer is returned and this registration
 * function is never called.
 *
 * However, if the plugin was not yet loaded, we call the dlopen() which
 * creates a plugin specific factory, which in turn calls the
 * plugin_factory::register_plugin() function, which finally calls this
 * very function to register the plugin in the global repository.
 *
 * The plugin_factory::register_plugin() is responsible for verifying that
 * the plugin name is valid.
 */
void plugin_repository::register_plugin(plugin::pointer_t p)
{
    p->f_filename = f_register_filename;

    f_plugins[f_register_filename] = p;
}



} // detail namespace
} // namespace cppthread
// vim: ts=4 sw=4 et
