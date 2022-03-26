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
#include    "cppthread/plugins_names.h"

#include    "cppthread/exception.h"
// #include    "cppthread/log.h"
// #include    "cppthread/guard.h"


// snapdev
//
#include    <snapdev/glob_to_list.h>
// #include    <snapdev/join_strings.h>
// #include    <snapdev/not_used.h>
#include    <snapdev/tokenize_string.h>


// C
//
#include    <unistd.h>

// last include
//
#include    <snapdev/poison.h>




namespace cppthread
{



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
 * \note
 * I considered accepting dashes (`-`) as well, but decided not to. Make
 * sure to use the underscore (`_`) if you want a separator.
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

    // No dashes! That's because we need to compare and we did not want
    // to compare with '-' == '_' (it's just too annoying to maintain).
    // Just make sure that you use '_' in your plugin names...
    //
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
    auto check = [&name](plugin_paths::path_t const & path)
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
        // if no paths were supplied, try the local folder
        //
        return check("./");
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
    snapdev::tokenize_string(names, set, ",", true, {' ', '\t', '\r', '\n'});
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
plugin_names::names_t plugin_names::names() const
{
    return f_names;
}


/** \brief Read all the available plugins in the specified paths.
 *
 * There are two ways that this class can be used:
 *
 * * First, the user specifies an exact list of names. In that case, you want
 * to use the add_name() function to add all the names from the user's list.
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
 * Note that the technical decoration is implemented internally. In other
 * words, the "lib" prefix and ".so" suffix are already handled by this
 * function. You do not need to use these at all.
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
    snapdev::glob_to_list<std::vector<std::string>> glob;

    std::size_t max(f_paths.size());
    for(std::size_t idx(0); idx < max; ++idx)
    {
        glob.read_path<
                  snapdev::glob_to_list_flag_t::GLOB_FLAG_IGNORE_ERRORS
                , snapdev::glob_to_list_flag_t::GLOB_FLAG_EMPTY
                , snapdev::glob_to_list_flag_t::GLOB_FLAG_PERIOD>(f_paths.at(idx) + "/" + prefix + "*" + suffix + ".so");
        glob.read_path<
                  snapdev::glob_to_list_flag_t::GLOB_FLAG_IGNORE_ERRORS
                , snapdev::glob_to_list_flag_t::GLOB_FLAG_EMPTY
                , snapdev::glob_to_list_flag_t::GLOB_FLAG_PERIOD>(f_paths.at(idx) + "/lib" + prefix + "*" + suffix + ".so");
        glob.read_path<
                  snapdev::glob_to_list_flag_t::GLOB_FLAG_IGNORE_ERRORS
                , snapdev::glob_to_list_flag_t::GLOB_FLAG_EMPTY
                , snapdev::glob_to_list_flag_t::GLOB_FLAG_PERIOD>(f_paths.at(idx) + "/*/" + prefix + "*" + suffix + ".so");
        glob.read_path<
                  snapdev::glob_to_list_flag_t::GLOB_FLAG_IGNORE_ERRORS
                , snapdev::glob_to_list_flag_t::GLOB_FLAG_EMPTY
                , snapdev::glob_to_list_flag_t::GLOB_FLAG_PERIOD>(f_paths.at(idx) + "/*/lib" + prefix + "*" + suffix + ".so");
    }

    for(auto const & n : glob)
    {
        push(n);
    }
}



} // namespace cppthread
// vim: ts=4 sw=4 et
