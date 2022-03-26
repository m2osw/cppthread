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
#include    "cppthread/plugins_paths.h"

#include    "cppthread/exception.h"
// #include    "cppthread/guard.h"


// snapdev
//
// #include    <snapdev/glob_to_list.h>
#include    <snapdev/join_strings.h>
// #include    <snapdev/not_used.h>
#include    <snapdev/tokenize_string.h>


// // C
// //
// #include    <dlfcn.h>

// last include
//
#include    <snapdev/poison.h>




namespace cppthread
{



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


/** \brief Change the allow-redirect flag.
 *
 * This function is used to switch the allow-redirect flag to true or false.
 * By default the flag is false.
 *
 * Setting the flag to true means that a user can define a relative path
 * outside of the current path (i.e. which starts with "../").
 *
 * \remarks
 * Most often, paths for plugin locations are full root paths so this flag
 * doesn't apply to those. Also, the canonicalization checks in memory
 * strings only. It will not verify that the path is not going outside of
 * the current path through softlink files.
 *
 * \param[in] allow  Whether to allow redirects in paths.
 *
 * \sa get_allow_redirects()
 * \sa canonicalize()
 */
void plugin_paths::set_allow_redirects(bool allow)
{
    f_allow_redirects = allow;
}


/** \brief Check whether redirects are allowed or not.
 *
 * This function returns true if redirects are allowed, false otherwise.
 *
 * When canonicalizing a path, a ".." outside of the current directory
 * is viewed as a redirect. These are not allowed by default for obvious
 * security reasons. When false and such a path is detected, the
 * canonicalize() function throws an error.
 *
 * \return true when redirects are allowed.
 *
 * \sa set_allow_redirects()
 */
bool plugin_paths::get_allow_redirects() const
{
    return f_allow_redirects;
}


/** \brief Canonicalize the input path.
 *
 * This function canonicalize the input path so that two paths referencing
 * the same files can easily be compared against each other.
 *
 * \note
 * We do not test the current local system (for one reason, the path may
 * not refer to a local file), so we will not detect soft links and relative
 * versus full path equivalents.
 *
 * \exception cppthread_invalid_error
 * The input \p path cannot be an empty string. Also, when the
 * allow-redirects flag (see the set_allow_redirects() function) is
 * set to false (the default), then the exception is raised if the path
 * starts with "../".
 *
 * \param[in] path  The path to be converted.
 *
 * \return The canonicalized path.
 *
 * \sa push()
 */
plugin_paths::path_t plugin_paths::canonicalize(path_t const & path)
{
    if(path.empty())
    {
        throw cppthread_invalid_error("path cannot be an empty string.");
    }

    bool const is_root(path[0] == '/');

    // canonicalize the path (exactly one "/" between each segment)
    //
    std::vector<std::string> segments;
    snapdev::tokenize_string(segments, path, "/", true);

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
                segments.erase(segments.begin());
                --idx;
            }
            else if(!f_allow_redirects)
            {
                throw cppthread_invalid_error(
                      "the path \""
                    + path
                    + "\" going outside of the allowed range.");
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
            ? '/' + snapdev::join_strings(segments, "/")
            : snapdev::join_strings(segments, "/");
}


/** \brief Add one path to this set of paths.
 *
 * This function is used to add a path to this set of paths.
 *
 * Before adding the new path, we make sure that it is not already defined
 * in the existing set. Adding the same path more than once is not useful.
 * Only the first instance would be useful and the second would generate
 * a waste of time.
 *
 * In many cases, you will want to use the add() function instead as it
 * is capable to add many paths separated by colons all at once.
 *
 * \note
 * The function calls canonicalize() on the input path. If the path is
 * considered invalid, then an exception is raised.
 *
 * \param[in] path  The path to be added.
 *
 * \sa add()
 * \sa canonicalize()
 */
void plugin_paths::push(path_t const & path)
{
    path_t const canonicalized(canonicalize(path));
    auto it(std::find(f_paths.begin(), f_paths.end(), canonicalized));
    if(it == f_paths.end())
    {
        f_paths.push_back(canonicalized);
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
    snapdev::tokenize_string(paths, set, ":", true, {' ', '\t', '\r', '\n'});
    for(auto const & p : paths)
    {
        push(p);
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



} // namespace cppthread
// vim: ts=4 sw=4 et
