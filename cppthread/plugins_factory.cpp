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
#include    "cppthread/plugins_factory.h"

#include    "cppthread/plugins_repository.h"
//#include    "cppthread/plugins.h"
// #include    "cppthread/guard.h"
// 
// 
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





/** \brief Initialize the plugin factory.
 *
 * This function creates a factory with a definition and an instance of the
 * plugin that the factory is expected to hold. This pointer is created
 * at the time the plugin specific factory is created on a dlopen() call.
 *
 * Later you can retrieve the plugin instance using the instance() function.
 * However, this is private to the plugin .cpp file. If you want a pointer
 * to the plugin, the plugin_collection::get_plugin_by_name() is probably
 * going to work better for you. Just make sure that the plugin_collection
 * is available through your user data (see plugin_collection::set_data()
 * for more details about that).
 *
 * \param[in] definition  The definition of the plugin.
 * \param[in] instance  The instance of the plugin.
 */
plugin_factory::plugin_factory(plugin_definition const & definition, std::shared_ptr<plugin> instance)
    : f_definition(definition)
    , f_plugin(instance)
{
}


/** \brief Verify that the plugin is ready for deletion.
 *
 * Whenever we unload the plugin (using dlclose()), the factor gets destroyed
 * and the plugin is expected to be ready for destruction which means no other
 * object still hold a reference to it.
 */
plugin_factory::~plugin_factory()
{
    // TODO: at this time this isn't working and it's not going to work any
    //       time soon, I'm afraid.
    //
    //if(f_plugin.use_count() != 1)
    //{
    //    std::cerr
    //        << "error: the plugin is still in use by objects other than the plugin factory, which is invalid at the time we are destroying the plugin."
    //        << std::endl;
    //    std::terminate();
    //}
}


/** \brief Return a reference to the plugin definition.
 *
 * Whenever you create a plugin, you have to create a plugin definition
 * which is a structure with various parameter such as the plugin version,
 * name, icon, description and dependencies.
 *
 * This function returns a reference to that definition.
 *
 * \return A reference to the plugin definition.
 */
plugin_definition const & plugin_factory::definition() const
{
    return f_definition;
}


/** \brief Retrieve a copy instance of the plugin.
 *
 * This function returns a copy of the plugin instance that the factory
 * allocated on construction. This is a shared pointer. Since the factory
 * can be destroyed by calling the dlclose() function, we can then detect
 * that the dlclose() was called too soon (i.e. that some other objects
 * still hold a reference to the plugin.)
 *
 * \return A shared pointer to the plugin managed by this plugin factory.
 */
std::shared_ptr<plugin> plugin_factory::instance() const
{
    return f_plugin;
}


/** \brief Regiter the specified plugin.
 *
 * This function gets called by the plugin factory of each plugin that gets
 * loaded by the dlopen() function.
 *
 * The function first verifies that the plugin name is a match. That test
 * should pretty much never fail.
 *
 * The function then calls the plugin_repository::register_plugin() function
 * which actually adds the plugin to the global list of plugins which allows
 * us to reference the same plugin in different collections.
 *
 * \param[in] name  The expected plugin name.
 * \param[in] p  The pointer to the plugin being registered.
 */
void plugin_factory::register_plugin(char const * name, plugin::pointer_t p)
{
    // `name` comes from the CPPTHREAD_PLUGIN_END macro
    // `p->name()` comes from the CPPTHREAD_PLUGIN_START macro
    //
    if(name != p->name())
    {
        // as long as you use the supplied macro, this should never ever
        // occur since the allocation of the plugin would fail if the
        // name do not match; that being said, you can attempt to create
        // two plugins in a single file and that is a case we do not support
        // and would result in such an error
        //
        throw cppthread_name_mismatch(                                                          // LCOV_EXCL_LINE
                "registering plugin named \""                                                   // LCOV_EXCL_LINE
                + p->name()                                                                     // LCOV_EXCL_LINE
                + "\" (from the plugin definition -- CPPTHREAD_PLUGIN_START), but expected \""  // LCOV_EXCL_LINE
                + name                                                                          // LCOV_EXCL_LINE
                + "\" (from the plugin factor definition -- CPPTHREAD_PLUGIN_END).");           // LCOV_EXCL_LINE
    }

    detail::plugin_repository::instance().register_plugin(p);
}


void plugin_factory::save_factory_in_plugin(plugin * p)
{
#ifdef _DEBUG
    if(f_definition.f_name != "server")
    {
        throw cppthread_logic_error("the save_factory_in_plugin() function is only to be used by the server plugin--other plugins are loaded and this save happens automatically");
    }
#endif
    p->f_factory = this;
}



} // namespace cppthread
// vim: ts=4 sw=4 et
