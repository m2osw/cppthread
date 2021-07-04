// Copyright (c) 2013-2021  Made to Order Software Corp.  All Rights Reserved
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


#include    "cppthread/log.h"
#include    "cppthread/guard.h"


// snapdev lib
//
#include    <snapdev/glob_to_list.h>
#include    <snapdev/join_strings.h>
#include    <snapdev/not_used.h>
#include    <snapdev/tokenize_string.h>


// C lib
//
#include    <dlfcn.h>

// last include
//
#include    <snapdev/poison.h>




namespace cppthread
{


namespace
{


/** \brief The global Plugin Repository.
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
class plugin_repository
{
public:
    static plugin_repository &  instance();
    plugin::pointer_t           get_plugin(plugin_names::filename_t const & filename);
    void                        register_plugin(plugin::pointer_t p);

private:
    mutex                       f_mutex = mutex();
    plugin::map_t               f_plugins = plugin::map_t();        // WARNING: this map is sorted by filename
    plugin_names::filename_t    f_register_filename = plugin_names::filename_t();
};



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
//SNAP_LOG_ERROR("note: registering plugin: \"")(name)("\"");

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
    f_plugins[p->filename()] = p;
}






} // no name namespace








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
    if(f_plugin.use_count() != 1)
    {
        std::cerr
            << "error: the plugin is still in use by objects other than the plugin factory, which is invalid at the time we are destroying the plugin."
            << std::endl;
        std::terminate();
    }
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
std::shared_ptr<plugin> plugin_factory::instance()
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
        throw cppthread_name_mismatch(
                "registering plugin named \""
                + p->name()
                + "\" (from the plugin definition -- CPPTHREAD_PLUGIN_START), but expected \""
                + name
                + "\" (from the plugin factor definition -- CPPTHREAD_PLUGIN_END).");
    }

    plugin_repository::instance().register_plugin(p);
}
















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




/** \brief Return the version of the plugin.
 *
 * This function returns the version of the plugin. This can be used to
 * verify that you have the correct version for a certain feature.
 *
 * \return A version structure which includes a major, minor and build version.
 */
version_t plugin::version() const
{
    return f_factory.definition().f_version;
}


/** \brief The last modification parameter.
 *
 * This function returns a timestamp representing the last time the plugin
 * was built.
 */
time_t plugin::last_modification() const
{
    return f_factory.definition().f_last_modification;
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
    return f_factory.definition().f_name;
}


/** \brief The filename of this plugin.
 *
 * This function returns the full path to the file that was loaded to make
 * this plugin work. This is the exact path that was used with the dlopen()
 * function.
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
    return f_factory.definition().f_description;
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
    return f_factory.definition().f_help_uri;
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
    return f_factory.definition().f_icon;
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
    return f_factory.definition().f_categorization_tags;
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
    return f_factory.definition().f_dependencies;
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
    return f_factory.definition().f_conflicts;
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
    return f_factory.definition().f_suggestions;
}





















/** \class plugin_paths
 * \brief The list of paths.
 *
 * The plugin_paths holds a list of paths that are used to search the
 * plugins. By default this list is empty which is viewed as a list
 * of having just "." as the path by the plugin_names::find_plugins().
 *
 * To add plugin paths, use the push(), add(), and set() functions to
 * see how to add new paths. In most cases, the set() function is ideal
 * if you read a list of paths from a configuration file.
 *
 * The set() function accepts a list of paths separated by colon (:)
 * characters. It is often used from an environment variable or a
 * parameter in a configuration file.
 */



/** \brief Get the number of paths defined in this set of paths.
 *
 * This function returns the number of paths this set has.
 */
std::size_t plugin_paths::size() const
{
    return f_paths.size();
}


/** \brief Get the path at the specified index.
 *
 * This function retrieves the path defined at \p idx.
 *
 * \param[in] idx  The index of the path you are trying to retrieve.
 *
 * \return The path defined at index \p idx. If \p idx is too large, then
 * this function returns an empty string.
 */
std::string plugin_paths::at(std::size_t idx) const
{
    if(idx >= f_paths.size())
    {
        return std::string();
    }

    return f_paths[idx];
}


/** \brief Canonicalize the input path.
 *
 * This function canonicalize the input path so that two paths referencing
 * the same files are eliminated.
 *
 * \note
 * We do not test the current local system (for one reason, the path may
 * not refer to a local file), so we will not detect soft links and relative
 * versus full path equivalents.
 *
 * \exception cppthread_invalid_error
 * The input \p path cannot be an empty string. Also, when the
 * \p allow_redirects parameter is set to false (the default), then the
 * exception is raised if the path starts with "../".
 *
 * \param[in] path  The path to be converted.
 * \param[in] allow_redirects  Whether a ".." is allowed at the top of the path.
 *
 * \return The canonicalized path.
 */
plugin_paths::path_t plugin_paths::canonicalize_path(path_t const & path, bool allow_redirects)
{
    if(path.empty())
    {
        throw cppthread_invalid_error("path cannot be an empty string");
    }

    bool const is_root(path[0] == '/');

    // canonicalize the path (exactly one "/" between each segment)
    //
    std::vector<std::string> segments;
    snap::tokenize_string(segments, path, "/", true);

    if(segments.empty())
    {
        return is_root
                ? std::string("/")
                : std::string(".");
    }

    for(std::size_t idx(0); idx < segments.size(); ++idx)
    {
        if(segments[idx] == ".")
        {
            segments.erase(segments.begin() + idx);
            --idx;
        }
        else if(segments[idx] == "..")
        {
            if(idx > 0
            && segments[idx - 1] != "..")
            {
                segments.erase(segments.begin() + idx);
                --idx;
                segments.erase(segments.begin() + idx);
                --idx;
            }
            else if(idx == 0
                 && is_root)
            {
                segments.erase(segments.begin() + idx);
            }
            else if(!allow_redirects)
            {
                throw cppthread_invalid_error(
                      "the path \""
                    + path
                    + "\" going outside of the allowed range");
            }
        }
    }

    if(segments.empty())
    {
        return is_root
                    ? std::string("/")
                    : std::string(".");
    }

    return is_root
            ? '/' + snap::join_strings(segments, "/")
            : snap::join_strings(segments, "/");
}


/** \brief Add one path to this set of paths.
 *
 * This function is used to add a path to this set of paths.
 *
 * Before adding the new path, we make sure that it is not already defined
 * in the existing set. Adding the same path more than once is not useful.
 * Only the first instance would be useful and the second generates a waste
 * of time.
 *
 * \exception cppthread_invalid_error
 * This function raises the cppthread_invalid_error exception if the input
 * string is the empty string (use "." for the current directory) or if
 * the string represents the root directory ("/").
 *
 * \param[in] path  The path to be added.
 */
void plugin_paths::push(path_t const & path)
{
    // make sure it's not empty
    //
    if(path.empty())
    {
        throw cppthread_invalid_error("you cannot add an empty path or just \"/\"; (1) the root path is not allowed; (2) use \".\" for the current directory.");
    }

    auto it(std::find(f_paths.begin(), f_paths.end(), path));
    if(it == f_paths.end())
    {
        f_paths.push_back(path);
    }
}


/** \brief Erase the specified path from this list.
 *
 * This function searches for the specified \p path and remove it from the
 * list. If not present in the list, then nothing happens.
 *
 * \param[in] path  The path to be removed.
 */
void plugin_paths::erase(std::string const & path)
{
    auto it(std::find(f_paths.begin(), f_paths.end(), path));
    if(it != f_paths.end())
    {
        f_paths.erase(it);
    }
}


/** \brief Add one set of paths to this set of paths.
 *
 * This function is used to add a set of colon separated paths defined in
 * one string. The function automatically separate each path at the colon
 * and adds the resulting paths to this object using the add() function.
 *
 * \note
 * The function further removes blanks (space, tab, newline, carriage
 * return) at the start and end of each path. Such characters should not
 * be supported at those locations by any sensible file systems anyway.
 *
 * \param[in] path  The path to be added.
 *
 * \sa add()
 */
void plugin_paths::add(std::string const & set)
{
    std::vector<std::string> paths;
    snap::tokenize_string(paths, set, ":", true, {' ', '\t', '\r', '\n'});
    for(auto const & p : paths)
    {
        push(p);
    }
}















/** \class plugin_names
 * \brief Manage a list of plugins to be loaded.
 *
 * Whenever you start a plugin system, you need to specify the list of
 * plugins you want to load. This is done by adding names to a
 * plugin_names object.
 *
 * In order to generate a list of plugin_names, you must first setup
 * a plugin_paths which defines the location where the plugins are
 * searched, which this class does each time a plugin name is added
 * to the object (see push(), add() and find_plugins() for details).
 *
 * See the plugin_collection class for more information and an example
 * on how to load plugins for a system.
 */




/** \brief Initialize a plugin_names object.
 *
 * The list of plugins has to be defined in a plugin_names object. It is done
 * this way because the list of plugin_paths becomes read-only once in the
 * list of paths of the plugin_names object. We actually make a deep copy of
 * the paths so we can be sure you can't add more paths later.
 *
 * The \p script_names parameter is used to determine whether the name
 * validation should prevent a plugin from using a reserved keyword (as
 * per ECMAScript). This is useful if you plan to have plugins used in
 * scripts and in there the plugins can be referenced by name.
 *
 * \param[in] paths  The list of paths to use to search the plugins.
 * \param[in] prevent_script_names  true if the plugin names are going to
 * be used in scripts.
 */
plugin_names::plugin_names(plugin_paths const & paths, bool prevent_script_names)
    : f_paths(paths)
    , f_prevent_script_names(prevent_script_names)
{
}


/** \brief Validate the name of a plugin.
 *
 * Plugin names are limited to the following regular expression:
 *
 * \code
 *     [A-Za-z_][A-Za-z0-9_]*
 * \endcode
 *
 * Further, the names can't be reserved keywords (as per ECMAScript) when the
 * \p script_names parameter of the constructor was set to true. So plugins
 * naming a keyword are rejected.
 *
 * \param[in] name  The string to verify as a plugin name.
 *
 * \return true if the name is considered valid.
 */
bool plugin_names::validate(name_t const & name)
{
    if(name.length() == 0)
    {
        return false;
    }

    if(name[0] != '_'
    && (name[0] < 'a' || name[0] > 'z')
    && (name[0] < 'A' || name[0] > 'Z'))
    {
        return false;
    }

    for(auto c : name)
    {
        if(c != '_'
        && (c < 'a' || c > 'z')
        && (c < 'A' || c > 'Z')
        && (c < '0' || c > '9'))
        {
            return false;
        }
    }

    // the name is considered to be a valid word, make sure it isn't an
    // ECMAScript reserved keyword if the user asked to prevent script names
    //
    if(f_prevent_script_names
    && is_emcascript_reserved(name))
    {
        return false;
    }

    return true;
}


/** \brief Check whether the input word is an ECMAScript reserved keyword.
 *
 * This function quickly checks whether the input \p word is considered a
 * reserved keyword by ECMAScript.
 *
 * The reserved keywords in ECMAScript 2022 are:
 *
 * \code
 * await break case catch class const continue debugger default delete do
 * else enum export extends false finally for function if import in
 * instanceof new null return super switch this throw true try typeof
 * var void while with yield
 * \endcode
 *
 * \param[in] word  The word to be checked.
 *
 * \return true if \p word is a reserved keyword.
 */
bool plugin_names::is_emcascript_reserved(std::string const & word)
{
    switch(word[0])
    {
    case 'a':
        if(word == "await")
        {
            return true;
        }
        break;

    case 'b':
        if(word == "break")
        {
            return true;
        }
        break;

    case 'c':
        if(word == "case"
        || word == "catch"
        || word == "class"
        || word == "const"
        || word == "continue")
        {
            return true;
        }
        break;

    case 'd':
        if(word == "debugger"
        || word == "default"
        || word == "delete"
        || word == "do")
        {
            return true;
        }
        break;

    case 'e':
        if(word == "else"
        || word == "enum"
        || word == "export"
        || word == "extends")
        {
            return true;
        }
        break;

    case 'f':
        if(word == "false"
        || word == "finally"
        || word == "for"
        || word == "function")
        {
            return true;
        }
        break;

    case 'i':
        if(word == "if"
        || word == "import"
        || word == "in"
        || word == "instanceof")
        {
            return true;
        }
        break;

    case 'n':
        if(word == "new"
        || word == "null")
        {
            return true;
        }
        break;

    case 'r':
        if(word == "return")
        {
            return true;
        }
        break;

    case 's':
        if(word == "super"
        || word == "switch")
        {
            return true;
        }
        break;

    case 't':
        if(word == "this"
        || word == "throw"
        || word == "true"
        || word == "try"
        || word == "typeof")
        {
            return true;
        }
        break;

    case 'v':
        if(word == "var"
        || word == "void")
        {
            return true;
        }
        break;

    case 'w':
        if(word == "while"
        || word == "with")
        {
            return true;
        }
        break;

    case 'y':
        if(word == "yield")
        {
            return true;
        }
        break;

    }

    return false;
}



/** \brief Convert a bare name in a filename.
 *
 * In most cases, your users will want to specify a bare name as a plugin
 * name and this library is then responsible for finding the corresponding
 * plugin on disk using the specified paths.
 *
 * This function transforms such a a bare name in a filename. It goes through
 * the list of paths (see the plugin_names() constructor) and stops once the
 * first matching plugin filename was found.
 *
 * If no such plugin is found, the function returns an empty string. Whether
 * to generate an error on such is your responsibility.
 *
 * \note
 * This function is used by add_name() which adds the name and the path in
 * the list of plugin names.
 *
 * \param[in] name  A bare plugin name.
 *
 * \return The full filename matching this bare plugin name or an empty string.
 */
plugin_names::filename_t plugin_names::to_filename(name_t const & name)
{
    auto check = [name](plugin_paths::path_t const & path)
    {
        // "path/<name>.so"
        //
        filename_t filename(path);
        filename += name;
        filename += ".so";
        if(access(filename.c_str(), R_OK | X_OK) == 0)
        {
            return filename;
        }

        // "path/lib<name>.so"
        //
        filename = path;
        filename += "lib";
        filename += name;
        filename += ".so";
        if(access(filename.c_str(), R_OK | X_OK) == 0)
        {
            return filename;
        }

        // "path/<name>/<name>.so"
        //
        filename = path;
        filename += name;
        filename += '/';
        filename += name;
        filename += ".so";
        if(access(filename.c_str(), R_OK | X_OK) == 0)
        {
            return filename;
        }

        // "path/<name>/lib<name>.so"
        //
        filename = path;
        filename += name;
        filename += "/lib";
        filename += name;
        filename += ".so";
        if(access(filename.c_str(), R_OK | X_OK) == 0)
        {
            return filename;
        }

        return filename_t();
    };

    std::size_t const max(f_paths.size());
    for(std::size_t idx(0); idx < max; ++idx)
    {
        plugin_paths::path_t path(f_paths.at(idx));
        path += '/';
        filename_t const filename(check(path));
        if(!filename.empty())
        {
            return filename;
        }
    }

    if(max == 0)
    {
        // try the local folder by default
        //
        return check(".");
    }

    return filename_t();
}


/** \brief Add the name of a plugin to be loaded.
 *
 * The function adds a plugin name which the load function will load once
 * it gets called. Only plugins that have their name added to this list will
 * be loaded.
 *
 * To fill the list with all the available plugins in all the paths you
 * added, you can use the find_plugins() function instead.
 *
 * \warning
 * This function assumes that you are done calling the add_path() function.
 *
 * \note
 * Adding the same name multiple times is allowed and does not create any
 * side effects. Only the first instance of the name is kept.
 *
 * \exception cppthread_invalid_error
 * The names are run through the validate_name() function to make sure they
 * are compatible with scripts. Also, we prevent the adding of the reserved
 * name called "server".
 *
 * \param[in] name  The name or filename of a plugin to be loaded.
 */
void plugin_names::push(name_t const & name)
{
    name_t n;
    filename_t fn;

    std::string::size_type pos(name.rfind('/'));
    if(pos != std::string::npos)
    {
        // we already received a path, extract the name and avoid calling
        // to_filename()
        //
        ++pos;
        if(name[pos + 0] == 'l'
        && name[pos + 1] == 'i'
        && name[pos + 2] == 'b')
        {
            pos += 3;
        }
        name_t::size_type l(name.length());
        if(l >= 3
        && name[l - 1] == 'o'
        && name[l - 2] == 's'
        && name[l - 3] == '.')
        {
            l -= 3;
        }
        n = name.substr(pos, l - pos);
        if(!validate(n))
        {
            throw cppthread_invalid_error(
                      "invalid plugin name in \""
                    + n
                    + "\" (from path \""
                    + name
                    + "\").");
        }
        fn = name;
    }
    else
    {
        if(!validate(name))
        {
            throw cppthread_invalid_error(
                      "invalid plugin name in \""
                    + name
                    + "\".");
        }
        fn = to_filename(name);
        if(fn.empty())
        {
            throw cppthread_not_found(
                      "plugin named \""
                    + name
                    + "\" not found in any of the specified paths.");
        }
        n = name;
    }

    if(n == "server")
    {
        throw cppthread_invalid_error("the name \"server\" is reserved for the main running process.");
    }

    f_names[n] = fn;
}


/** \brief Add a comma separated list of names.
 *
 * If you offer your users a way to define a list of names separated by
 * commas, this function is exactly what you need to add all those names
 * in one go.
 *
 * The names are also trimmed of blanks (spaces, tables, newline, and
 * carriage returns are all removed).
 *
 * \param[in] set  A comma separated list of plugin names.
 */
void plugin_names::add(std::string const & set)
{
    std::vector<std::string> names;
    snap::tokenize_string(names, set, ",", true, {' ', '\t', '\r', '\n'});
    for(auto const & n : names)
    {
        push(n);
    }
}


/** \brief Retrieve the map of name/filename.
 *
 * This function is used to retrieve a copy of the map storing the plugin
 * name and filename pairs. This represents the complete list of plugins
 * to be loaded.
 *
 * \note
 * When the find_plugins() function was called to generate the list of
 * name/filename pairs, then it is known that the filename does exist on
 * disk. However, this doesn't mean the plugin can be loaded (or even that
 * the file is indeed a cppthread compatible plugin).
 *
 * \note
 * The push() function, when called with a full filename, will extract the
 * name from that filename and save that directly to the map. In other words,
 * it does not verify whether the file exists.
 *
 * \return The map of name/filename pairs.
 */
plugin_names::names_t plugin_names::get_names() const
{
    return f_names;
}


/** \brief Read all the available plugins in the specified paths.
 *
 * There are two ways that this class can be used:
 *
 * * First, the user specifies an exact list of names. In that case, you want
 * to use the add_name() function to add all the names from your list.
 *
 * * Second, the user adds the plugins to a directory and the idea is to
 * have all of them loaded. In this second case, you use the find_plugins()
 * which runs glob() in those paths to retrieve all the plugins that can
 * be loaded.
 *
 * Note that this function only finds the plugins. It doesn't load them. To
 * then load all of these plugins, use the load_plugins() function.
 *
 * The function accepts a prefix and a suffix which are used to search for
 * the plugins. These are expected to be used when you are looking for a
 * set of plugins that make use of such prefix/suffix to group said plugins
 * in some way. For example, you could have output plugins that use a prefix
 * such as:
 *
 * \code
 * output_bare
 * output_decorated
 * output_html
 * output_json
 * \endcode
 *
 * So calling this function with the prefix set to `output_` will only load
 * these four plugins and ignore the others (i.e. the `input_`, for example).
 *
 * \warning
 * You must call the add_path() function with all the paths that you want
 * to support before calling this function.
 *
 * \param[in] prefix  The prefix used to search the plugins.
 * \param[in] suffix  The suffix used to search the plugins.
 */
void plugin_names::find_plugins(name_t const & prefix, name_t const & suffix)
{
    snap::glob_to_list<std::vector<std::string>> glob;

    std::size_t max(f_paths.size());
    for(std::size_t idx(0); idx < max; ++idx)
    {
        glob.read_path<
                  snap::glob_to_list_flag_t::GLOB_FLAG_IGNORE_ERRORS
                , snap::glob_to_list_flag_t::GLOB_FLAG_PERIOD>(f_paths.at(idx) + "/" + prefix + "*" + suffix + ".so");
        glob.read_path<
                  snap::glob_to_list_flag_t::GLOB_FLAG_IGNORE_ERRORS
                , snap::glob_to_list_flag_t::GLOB_FLAG_PERIOD>(f_paths.at(idx) + "/*/" + prefix + "*" + suffix + ".so");
    }

    for(auto n : glob)
    {
        push(n);
    }
}
















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
 *     plugin_paths p;
 *     p.add("/usr/lib/snaplogger/plugins:/usr/local/lib/snaplogger/plugins");
 *
 *     plugin_names n(p, true);
 *     n.add("network, cloud-system");
 *     // or:  n.find_plugins();   to read all the plugins
 *
 *     plugin_collection c(n);
 *     c.set_data(&my_app);
 *     c.load_plugins();
 * \endcode
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
 * \param[in] names  A list of plugins to be loaded.
 *
 * \sa set_data();
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
 * \return true if the loading worked on all the plugins, false otherwise.
 */
bool plugin_collection::load_plugins()
{
    guard lock(f_mutex);

    // TODO/TBD? add a "server" plugin which represents the main process
    //f_plugins.insert("server", server.get());

    plugin_repository & repository(plugin_repository::instance());
    bool good(true);
    for(auto const & name_filename : f_names.get_names())
    {
        // the main process is considered a "server" and it will eventually
        // be added to the list under that name, so we can't allow this name
        // here
        //
        if(name_filename.first == "server")
        {
            log << log_level_t::error
                << "a plugin cannot be called \"server\"."
                << end;
            good = false;
            continue;
        }

        plugin::pointer_t p(repository.get_plugin(name_filename.second));
        if(p == nullptr)
        {
            log << cppthread::log_level_t::fatal
                << "plugin \""
                << name_filename.first
                << "\" not found."
                << end;
            good = false;
            continue;
        }

        string_set_t const conflicts1(p->conflicts());
        for(auto & op : f_plugins)
        {
            // the conflicts can be indicated in either direction so we
            // have to test both unless one is true then we do not have to
            // test the other
            //
            bool in_conflict(conflicts1.find(op->name()) != conflicts1.end());
            if(!in_conflict)
            {
                string_set_t const conflicts2(op->conflicts());
                in_conflict = conflicts2.find(op->name()) != conflicts2.end();
            }
            if(in_conflict)
            {
                log << cppthread::log_level_t::fatal
                    << "plugin \""
                    << op->name()
                    << "\" is in conflict with \""
                    << name_filename.first
                    << "\"."
                    << end;
                good = false;
                continue;
            }
        }

        f_plugins.push_back(p);
        f_plugins_by_name[name_filename.first] = p;
    }

    // set the f_ordered_plugins with the default order as alphabetical,
    // although we check dependencies to properly reorder as expected
    // by what each plugin tells us what its dependencies are
    //
    for(auto const & p : f_plugins)
    {
        auto it(std::find_if(
                f_ordered_plugins.begin(),
                f_ordered_plugins.end(),
                [&p](auto const & plugin)
                {
                    return plugin->dependencies().find(p->name()) != plugin->dependencies().end();
                }));
        if(it != f_ordered_plugins.end())
        {
            f_ordered_plugins.insert(it, p);
        }
        else
        {
            f_ordered_plugins.push_back(p);
        }
    }

    // bootstrap() functions have to be called in order to get all the
    // signals registered in order! (YES!!! This one for() loop makes
    // all the signals work as expected by making sure they are in a
    // very specific order)
    //
    for(auto const & p : f_ordered_plugins)
    {
        p->bootstrap(f_data);
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
plugin::pointer_t plugin_collection::get_plugin_by_name(std::string const & name)
{
    auto it(f_plugins_by_name.find(name));
    if(it != f_plugins_by_name.end())
    {
        return it->second;
    }

    return plugin::pointer_t();
}






} // namespace cppthread
// vim: ts=4 sw=4 et
