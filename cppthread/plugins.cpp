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
#include    "cppthread/plugins.h"

#include    "cppthread/plugins_factory.h"

#include    "cppthread/log.h"
#include    "cppthread/guard.h"


// snapdev
//
#include    <snapdev/glob_to_list.h>
#include    <snapdev/join_strings.h>
#include    <snapdev/not_used.h>
#include    <snapdev/tokenize_string.h>


// C
//
#include    <dlfcn.h>

// last include
//
#include    <snapdev/poison.h>




namespace cppthread
{



/** \class plugin
 * \brief The plugin class to manage the plugin parameters.
 *
 * Each plugin gets loaded with the dlopen() system function. The plugin
 * is created by the plugin factory.
 *
 * The plugin includes a description which is expected to be created with
 * the CPPTHREAD_PLUGIN_START() macro. This will automatically verify that
 * the parameters are mostly correct at compile time.
 *
 * The end of the description happens by adding the CPPTHREAD_PLUGIN_END()
 * macro which is then followed by your own plugin functions.
 */




/** \brief Create the server plugin.
 *
 * This constructor is here for the server plugin. All the other plugins
 * are expected to be created using a factory and that factory's pointer
 * is available to create the plugin with the other constructor.
 */
plugin::plugin()
{
}


/** \brief Initialize the plugin with its factory.
 *
 * This constructor saves the plugin factory pointer. This is used by the
 * various functions returning plugin definition parameters.
 *
 * \param[in] factory  The factory that created this plugin.
 */
plugin::plugin(plugin_factory const & factory)
    : f_factory(&factory)
{
}


/** \brief For the virtual table.
 *
 * We want the plugin class to have a virtual table so we have a virtual
 * destructor.
 *
 * \note
 * At the moment this destructor is never called. We'll want to look into
 * a proper way to dlclose() plugins at some point.
 */
plugin::~plugin()           // LCOV_EXCL_LINE
{                           // LCOV_EXCL_LINE
}                           // LCOV_EXCL_LINE


/** \brief Return the version of the plugin.
 *
 * This function returns the version of the plugin. This can be used to
 * verify that you have the correct version for a certain feature.
 *
 * \return A version structure which includes a major, minor and build version.
 */
version_t plugin::version() const
{
    return f_factory->definition().f_version;
}


/** \brief The last modification parameter.
 *
 * This function returns a timestamp representing the last time the plugin
 * was built.
 */
time_t plugin::last_modification() const
{
    return f_factory->definition().f_last_modification;
}


/** \brief The name of the plugin.
 *
 * This function returns the name of the plugin.
 *
 * \note
 * When registering the plugin, the code verifies that this parameter returns
 * the same name as the named used to load the plugin. In other words, we
 * can trust this parameter.
 *
 * \return The name of the plugin.
 */
std::string plugin::name() const
{
    return f_factory->definition().f_name;
}


/** \brief The filename of this plugin.
 *
 * This function returns the full path to the file that was loaded to make
 * this plugin work. This is the exact path that was used with the dlopen()
 * function.
 *
 * \note
 * Whenever the plugin registers itself via the plugin_repository class,
 * the plugin_repository takes that chance to save the filename to the
 * plugin class. This path is one to one the one used with the dlopen()
 * function call.
 *
 * \return The path to the plugin file.
 */
std::string plugin::filename() const
{
    return f_filename;
}


/** \brief A brief description.
 *
 * This function returns the brief description for this plugin.
 *
 * \return A brief description of the plugin.
 */
std::string plugin::description() const
{
    return f_factory->definition().f_description;
}


/** \brief A URI to a document explaining how to use this plugin.
 *
 * This function returns a URI which one can use to send the user to a website
 * where the user can read about this plugin.
 *
 * \return A URI to this plugin help page(s).
 */
std::string plugin::help_uri() const
{
    return f_factory->definition().f_help_uri;
}


/** \brief The icon representing this plugin.
 *
 * The function returns a filename, a resource name, or a URL to a file
 * representing this plugin. In other words, the Logo representing this
 * plugin.
 *
 * \return The filename, resource name, or URL to an image.
 */
std::string plugin::icon() const
{
    return f_factory->definition().f_icon;
}


/** \brief Return a list of tags.
 *
 * This function returns a list of tags that categorizes the plugin in various
 * ways. For example, all plugins that deal with emails can use the tag
 * "email".
 *
 * \return A set of strings representing categories or tags.
 */
string_set_t plugin::categorization_tags() const
{
    return f_factory->definition().f_categorization_tags;
}


/** \brief List of dependencies.
 *
 * This function returns a list of dependencies that this plugin needs to
 * run properly. You do not have to specify all the dependencies when you
 * want to load a plugin. The plugin_collection::load_plugins() will
 * automatically add those dependencies as it finds them.
 *
 * \return The list of plugin names that need to be loaded for this plugin
 * to work properly.
 */
string_set_t plugin::dependencies() const
{
    return f_factory->definition().f_dependencies;
}


/** \bfief List of conflicts.
 *
 * This function returns a set of strings with names of plugins that are in
 * conflict with this plugin. For example, you may create two plugins so
 * send emails and installing both would mean that emails would be sent
 * twice. Using this makes sure that you can't actually load both plugins
 * simultaneously (if that happens, then an error occurs and the load
 * fails).
 *
 * \return A list of plugin names that are in conflict with this plugin.
 */
string_set_t plugin::conflicts() const
{
    return f_factory->definition().f_conflicts;
}


/** \brief List of suggestions.
 *
 * This function returns a set of strings with various suggestions of other
 * plugins that add functionality to this plugin.
 *
 * \return The list of suggestions for this plugin.
 */
string_set_t plugin::suggestions() const
{
    return f_factory->definition().f_suggestions;
}


/** \brief Path to this specific plugin settings.
 *
 * This function returns a path to the plugin settings. The settings may
 * be in a database or on disk. The path will depend on how the plugins
 * are used.
 *
 * \return The path to this plugin settings.
 */
std::string plugin::settings_path() const
{
    return f_factory->definition().f_settings_path;
}


plugin_collection * plugin::collection() const
{
    return f_collection;
}


/** \brief Give the plugin a chance to properly initialize itself.
 *
 * The order in which plugins are loaded is generally just alphabetical
 * which in most cases is not going to cut it well when initializing them.
 * Instead, the library offers a dependency list in each plugin so plugins
 * that are depended on can be initialized first (i.e. if A depends on B,
 * then B gets initialized first).
 *
 * The bootstrap() is the function that gets called once all the plugins
 * of a collection were loaded. This gives you the ability to properly
 * initialize your plugins.
 *
 * The default bootstrap() function does nothing at the moment.
 *
 * \sa plugin_collection::set_data()
 * \sa plugin_collection::get_data<T>()
 */
void plugin::bootstrap()
{
}


/** \brief Allow for updates.
 *
 * After a website loads a plugin, it can call this function to update the
 * database to the lastest version of the plugin. In most cases, no such
 * function is required, but there are a few cases where it is a must.
 *
 * The input and output are dates in seconds of when the plugin was updated.
 * If the plugin was not updated, then nothing happens in the function
 * and the same date is returned.
 *
 * If multiple updates happened then all of them get applied.
 *
 * \note
 * The default implementation does nothing.
 *
 * \param[in] last_updated  The last time it was updated.
 *
 * \return The date of the last update.
 */
time_t plugin::do_update(time_t last_updated)
{
    return last_updated;
}



} // namespace cppthread
// vim: ts=4 sw=4 et
