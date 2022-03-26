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
#include    "cppthread/plugins_collection.h"

#include    "cppthread/plugins_repository.h"

#include    "cppthread/log.h"
#include    "cppthread/guard.h"


// // snapdev
// //
// #include    <snapdev/glob_to_list.h>
// #include    <snapdev/join_strings.h>
// #include    <snapdev/not_used.h>
// #include    <snapdev/tokenize_string.h>


// C++
//
#include    <algorithm>


// // C
// //
// #include    <dlfcn.h>

// last include
//
#include    <snapdev/poison.h>




namespace cppthread
{



/** \class plugin_collection
 * \brief Handle a collection of plugins.
 *
 * In order to create a plugin_collection, you first need to have a list
 * of plugin names (and filenames), a.k.a. a plugin_names object.
 *
 * In order to create a plugin_names object, you first need to create a
 * plugin_paths object. These are lists of paths where the plugin files
 * are searched before being loaded.
 *
 * The order is important and prevents you from modifying the paths while
 * adding names, and modifying the paths and names which building the
 * collection of plugins.
 *
 * \code
 *     // first make sure your main daemon object derives from
 *     // cppthread::server; this way you can add it as the "server"
 *     // plugin and then the  other plugins depend on it
 *     //
 *     class daemon
 *         : public cppthread::server
 *     {
 *     public:
 *         ...
 *     };
 *
 *     // create your daemon (in most cases we do that in our main()
 *     // function, notice, though that we want a shared pointer
 *     //
 *     daemon::pointer_t d(std::make_shared<daemon>());
 *
 *     ...
 *
 *     plugin_paths p;
 *     p.add("/usr/local/lib/snaplogger/plugins:/usr/lib/snaplogger/plugins");
 *
 *     plugin_names n(p, true);
 *     n.add("network, cloud-system");
 *     // or:  n.find_plugins();   to read all the plugins
 *
 *     plugin_collection c(n);
 *     c.set_data(&my_app);
 *     c.load_plugins(d);   // your daemon is passed down to all plugins now
 * \endcode
 *
 * The set_data() is still available, but the data is not passed
 * down through plugin::bootstrap(). Instead, you use the plugin::collection()
 * function and the get_data<T>() template. In most cases, you may be able to
 * use the get_server() instead.
 *
 * The plugin_names makes a deep copy of the plugin_paths.
 *
 * The plugin_collection makes a deep copy of the plugin_names (meaning that
 * it also makes a new copy of the plugin_paths).
 */



/** \brief Initialize a collection of plugins.
 *
 * The input parameter is a list of plugin names that can be loaded with
 * the load_plugins(). Before calling the load_plugins() function, you
 * may want to call the set_data() function in order to define the pointer
 * that will be passed to the bootstrap() function.
 *
 * The f_names is a copy of your \p names parameter. That copy may be modified
 * if some of the plugins have dependencies that were not listed in the input
 * \p names object. In other words, if A depends on B, you only need to
 * specify A as the plugin you want to load. The load_plugins() function
 * will automatically know that it has to then load B.
 *
 * \param[in] names  A list of plugins to be loaded.
 *
 * \sa set_data();
 * \sa load_plugins();
 */
plugin_collection::plugin_collection(plugin_names const & names)
    : f_names(names)
{
}


/** \brief Set user data for when the bootstrap function gets called.
 *
 * If you need a reference back to another object from your plugins, set
 * it in here. In the Snap! environment, we use the snap_child object.
 * That way all the plugins have access to everything in the server.
 *
 * \param[in] data  Your data pointer.
 *
 * \sa plugin::bootstrap()
 */
void plugin_collection::set_data(void * data)
{
    f_data = data;
}


/** \brief Load all the plugins in this collection.
 *
 * When you create a collection, you pass a list of names (via the
 * plugin_names object). This function uses that list to load all the
 * corresponding plugins from disk.
 *
 * The constructor of the plugins will be called and you can do local
 * initialization as required at that point. Do not attempt to initialize
 * your system just yet. Instead, this function will also call the
 * plugin::bootstrap() function with your user data (see set_data() for
 * details about that parameter). In that function, you can start deep
 * initialization that involves other plugins, such as dependencies.
 *
 * The order in which the bootstrap() functions will be called is well
 * defined via the list of ordered plugins. The order makes use of the
 * plugin dependency list (see plugin::dependencies() for details) and
 * the name of the plugin. If two plugins do not depend on each other,
 * then they get sorted alphabeticall (so A always comes before B unless
 * A depends on B, then B would be initialized first).
 *
 * The order in which the plugins get initialized is very important if
 * you use the signal system since it means that the order in which signals
 * get processed depends on the order in which the plugins were sorted.
 * At the moment, the one drawback is that all the signals of one plugin
 * get added at the same time (via the bootstrap() function) which means
 * that signal A:S1 will not happen before B:S1 if A is initialized first.
 * In our experence, it has been pretty reliable in most cases. We had a
 * very few cases were something would not happen quite in order and we
 * had to add another message to fix the issue (i.e. if A:S1 happens
 * before B:S1 but you need A to react after B:S1 made some changes to
 * the state, you can have a second message A:S2 sent afterward, and
 * B:S2 can be ignored).
 *
 * \param[in] s  The server, the "plugin" considered the root plugin.
 *
 * \return true if the loading worked on all the plugins, false otherwise.
 */
bool plugin_collection::load_plugins(server::pointer_t s)
{
    guard lock(f_mutex);

    // this is a bit ugly but it allows use to define a root like plugin
    // which is the main process code; we call it a server because in
    // most cases it will be a server/daemon type of process which makes
    // use of plugins (at least in our environment)
    //
    // so in the following we (1) create a server factory manually
    // and (2) register the server as f_server and f_plugins_by_name["server"]
    //
    f_server = s;
    detail::g_plugin_server_factory = new detail::plugin_server_factory(s);
    f_plugins_by_name["server"] = s;

#ifdef _DEBUG
    if(s->name() != "server")
    {
        throw cppthread_logic_error("the name in the plugin_server_factory definition must be \"server\".");
    }
#endif

    detail::plugin_repository & repository(detail::plugin_repository::instance());
    bool changed(true);
    bool good(true);
    while(changed)
    {
        changed = false;

        plugin_names::names_t names(f_names.names());
        for(auto const & name_filename : names)
        {
            // the main process is considered to be the "server" plugin and it
            // will eventually be added to the list under that name, so we can't
            // allow this name here
            //
            // Note: this should not happen since we don't allow the addition of
            // the "server" name to the list of names (see plugin_names::push()
            // for details)
            //
            if(name_filename.first == "server")
            {
                log << log_level_t::error                           // LCOV_EXCL_LINE
                    << "a plugin cannot be called \"server\"."      // LCOV_EXCL_LINE
                    << end;                                         // LCOV_EXCL_LINE
                good = false;                                       // LCOV_EXCL_LINE
                continue;                                           // LCOV_EXCL_LINE
            }

            plugin::pointer_t p(repository.get_plugin(name_filename.second));
            if(p == nullptr)
            {
                log << cppthread::log_level_t::fatal
                    << "loaded file \""
                    << name_filename.second
                    << "\" for plugin \""
                    << name_filename.first
                    << "\", but the plugin was not found (name mismatch? plugin not defined?)."
                    << end;
                good = false;
                continue;
            }

            // give plugin access back to the collection and thus:
            //
            //  * the server
            //  * the user data
            //  * other plugins (useful to know whether a plugin exists)
            //
            p->f_collection = this;

            string_set_t const conflicts1(p->conflicts());
            for(auto & op : f_plugins_by_name)
            {
                // the conflicts can be indicated in either direction so we
                // have to test both unless one is true then we do not have to
                // test the other
                //
                bool in_conflict(conflicts1.find(op.first) != conflicts1.end());
                if(!in_conflict)
                {
                    string_set_t const conflicts2(op.second->conflicts());
                    in_conflict = conflicts2.find(op.first) != conflicts2.end();
                }
                if(in_conflict)
                {
                    log << cppthread::log_level_t::fatal
                        << "plugin \""
                        << op.first
                        << "\" is in conflict with \""
                        << name_filename.first
                        << "\"."
                        << end;
                    good = false;
                    continue;
                }
            }

            string_set_t const dependencies(p->dependencies());
            for(auto & d : dependencies)
            {
                if(names.find(d) == names.end())
                {
                    f_names.push(d);
                    changed = true;
                }
            }

            f_plugins_by_name[name_filename.first] = p;
        }
    }

    // set the f_ordered_plugins with the default order as alphabetical,
    // although we check dependencies to properly reorder as expected
    // by what each plugin tells us what its dependencies are
    //
    for(auto const & p : f_plugins_by_name)
    {
        auto it(std::find_if(
                f_ordered_plugins.begin(),
                f_ordered_plugins.end(),
                [&p](auto const & plugin)
                {
                    return plugin->dependencies().find(p.first) != plugin->dependencies().end();
                }));
        if(it != f_ordered_plugins.end())
        {
            f_ordered_plugins.insert(it, p.second);
        }
        else
        {
            f_ordered_plugins.push_back(p.second);
        }
    }

    // bootstrap() functions have to be called in order to get all the
    // signals registered in order!
    //
    // This one for() loop makes all the signals work as expected by
    // making sure they are in a very specific order as defined by
    // your dependency list.
    //
    for(auto const & p : f_ordered_plugins)
    {
        p->bootstrap();
    }

    return good;
}


/** \brief Check whether a given plugin is already loaded.
 *
 * This function checks to see whether the named plugin was loaded. If so
 * the function returns true.
 *
 * In your code or even scripts, this function can be very useful to check
 * whether a plugin is available before trying to access its functionality.
 * So in other words, you can make certain plugins optional and still have
 * a fully functional system, just with less capabilities.
 *
 * \param[in] name  The name of the plugin to check for.
 *
 * \return True if the plugin is found, false otherwise.
 */
bool plugin_collection::is_loaded(std::string const & name) const
{
    return f_plugins_by_name.find(name) != f_plugins_by_name.end();
}



} // namespace cppthread
// vim: ts=4 sw=4 et
