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
#include    "cppthread/mutex.h"

#include    "cppthread/exception.h"
#include    "cppthread/version.h"       // used in CPPTHREAD_PLUGIN_START()


// snapdev lib
//
#include    <snapdev/not_used.h>


// C++ set
//
#include    <iostream>
#include    <map>
#include    <memory>
#include    <set>
#include    <string>
#include    <vector>


namespace cppthread
{


namespace detail
{
class plugin_repository;
} // detail namespace


typedef std::set<std::string>           string_set_t;


template<int N>
constexpr char const * validate_name(char const (&str)[N])
{
    static_assert(N - 1 > 0);

    if(str[0] != '_'
    && (str[0] < 'a' || str[0] > 'z')
    && (str[0] < 'A' || str[0] > 'Z'))
    {
        throw cppthread_logic_error(
                    "first character of name \""
                  + std::string(str)
                  + "\" not valid.");
    }

    for(int i(1); i < N - 1; ++i)
    {
        if(str[i] != '_'
        && (str[i] < 'a' || str[i] > 'z')
        && (str[i] < 'A' || str[i] > 'Z')
        && (str[i] < '0' || str[i] > '9'))
        {
            throw cppthread_logic_error(
                    "character #"
                  + std::to_string(i)
                  + " ("
                  + str[i]
                  + ") of name \""
                  + str
                  + "\" not valid.");
        }
    }

    return str;
}


constexpr time_t validate_date(time_t date)
{
    // any new plugin must be created after this date (about 2021/06/22 10:30)
    //
    if(date < 1624382757LL)
    {
        throw cppthread_out_of_range("plugin dates are expected to be at least 2021/06/22 10:30");
    }
    return date;
}


template<typename T>
constexpr void validate_version(T const major, T const minor)
{
    if(major <= 0 && minor <= 0)
    {
        throw cppthread_logic_error("the plugin version cannot be 0.0 or use negative numbers.");
    }
}




struct version_t
{
                        constexpr version_t()
                            : f_major(0)
                            , f_minor(0)
                            , f_patch(0)
                        {
                        }

                        version_t(
                                  std::int32_t major
                                , std::int32_t minor
                                , std::int32_t patch = 0)
                            : f_major(major)
                            , f_minor(minor)
                            , f_patch(patch)
                        {
                            validate_version(major, minor);
                        }

    std::int32_t        f_major = 0;
    std::int32_t        f_minor = 0;
    std::int32_t        f_patch = 0;
};


struct plugin_definition
{
    version_t                           f_version = version_t();
    version_t                           f_library_version = version_t();
    time_t                              f_last_modification = 0;        // uses the compilation date & time converted to a Unix date
    std::string                         f_name = std::string();
    std::string                         f_description = std::string();
    std::string                         f_help_uri = std::string();
    std::string                         f_icon = std::string();
    string_set_t                        f_categorization_tags = string_set_t();
    string_set_t                        f_dependencies = string_set_t();
    string_set_t                        f_conflicts = string_set_t();
    string_set_t                        f_suggestions = string_set_t();
    std::string                         f_settings_path = std::string();
};





template<typename T>
class plugin_definition_value
{
public:
    typedef T   value_t;

    constexpr plugin_definition_value<T>(T const v)
        : f_value(v)
    {
    }

    constexpr value_t get() const
    {
        return f_value;
    }

private:
    value_t     f_value = value_t();
};




class plugin_version
    : public plugin_definition_value<version_t>
{
public:
    constexpr plugin_version()
        : plugin_definition_value<version_t>(version_t())
    {
    }

    constexpr plugin_version(version_t version)
        : plugin_definition_value<version_t>(version)
    {
    }
};


class plugin_library_version
    : public plugin_definition_value<version_t>
{
public:
    constexpr plugin_library_version()
        : plugin_definition_value<version_t>(version_t())
    {
    }

    constexpr plugin_library_version(version_t version)
        : plugin_definition_value<version_t>(version)
    {
    }
};


class plugin_last_modification
    : public plugin_definition_value<time_t>
{
public:
    constexpr plugin_last_modification()
        : plugin_definition_value<time_t>(time_t())
    {
    }

    constexpr plugin_last_modification(time_t const last_modification)
        : plugin_definition_value<time_t>(validate_date(last_modification))
    {
    }
};


class plugin_name
    : public plugin_definition_value<char const *>
{
public:
    constexpr plugin_name()
        : plugin_definition_value<char const *>(nullptr)
    {
    }

    template<int N>
    constexpr plugin_name(char const (&name)[N])
        : plugin_definition_value<char const *>(validate_name(name))
    {
    }
};


class plugin_description
    : public plugin_definition_value<char const *>
{
public:
    constexpr plugin_description()
        : plugin_definition_value<char const *>("")
    {
    }

    constexpr plugin_description(char const * description)
        : plugin_definition_value<char const *>(description)
    {
    }
};


class plugin_help_uri
    : public plugin_definition_value<char const *>
{
public:
    constexpr plugin_help_uri()
        : plugin_definition_value<char const *>("")
    {
    }

    constexpr plugin_help_uri(char const * help_uri)
        : plugin_definition_value<char const *>(help_uri)
    {
    }
};


class plugin_icon
    : public plugin_definition_value<char const *>
{
public:
    constexpr plugin_icon()
        : plugin_definition_value<char const *>("")
    {
    }

    constexpr plugin_icon(char const * icon)
        : plugin_definition_value<char const *>(icon)
    {
    }
};


class plugin_categorization_tag
    : public plugin_definition_value<char const *>
{
public:
    template<int N>
    constexpr plugin_categorization_tag(char const (&tag)[N])
        : plugin_definition_value<char const *>(validate_name(tag))
    {
    }
};


class plugin_dependency
    : public plugin_definition_value<char const *>
{
public:
    template<int N>
    constexpr plugin_dependency(char const (&dependency)[N])
        : plugin_definition_value<char const *>(validate_name(dependency))
    {
    }
};


class plugin_conflict
    : public plugin_definition_value<char const *>
{
public:
    template<int N>
    constexpr plugin_conflict(char const (&conflict)[N])
        : plugin_definition_value<char const *>(validate_name(conflict))
    {
    }
};


class plugin_suggestion
    : public plugin_definition_value<char const *>
{
public:
    template<int N>
    constexpr plugin_suggestion(char const (&suggestion)[N])
        : plugin_definition_value<char const *>(validate_name(suggestion))
    {
    }
};


class plugin_settings_path
    : public plugin_definition_value<char const *>
{
public:
    constexpr plugin_settings_path()
        : plugin_definition_value<char const *>("")
    {
    }

    template<int N>
    constexpr plugin_settings_path(char const (&settings_path)[N])
        : plugin_definition_value<char const *>(validate_name(settings_path))
    {
    }
};






template<typename T, typename F, class ...ARGS>
constexpr typename std::enable_if<std::is_same<T, F>::value, typename T::value_t>::type find_plugin_information(F first, ARGS ...args)
{
    snapdev::NOT_USED(args...);
    return first.get();
}


template<typename T, typename F, class ...ARGS>
constexpr typename std::enable_if<!std::is_same<T, F>::value, typename T::value_t>::type find_plugin_information(F first, ARGS ...args)
{
    snapdev::NOT_USED(first);
    return find_plugin_information<T>(args...);
}





template<typename T, typename F>
typename std::enable_if<std::is_same<T, F>::value, string_set_t>::type find_plugin_set(F first)
{
    string_set_t s;
    s.insert(first.get());
    return s;
}


template<typename T, typename F>
typename std::enable_if<!std::is_same<T, F>::value, string_set_t>::type find_plugin_set(F first)
{
    snapdev::NOT_USED(first);
    return string_set_t();
}


template<typename T, typename F, class ...ARGS>
typename std::enable_if<std::is_same<T, F>::value && (sizeof...(ARGS) > 0), string_set_t>::type find_plugin_set(F first, ARGS ...args)
{
    string_set_t s(find_plugin_set<T>(args...));
    s.insert(first.get());
    return s;
}


template<typename T, typename F, class ...ARGS>
typename std::enable_if<!std::is_same<T, F>::value && (sizeof...(ARGS) > 0), string_set_t>::type find_plugin_set(F first, ARGS ...args)
{
    snapdev::NOT_USED(first);
    return find_plugin_set<T>(args...);
}



template<class ...ARGS>
constexpr plugin_definition define_plugin(ARGS ...args)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
    plugin_definition def =
    {
        .f_version =                find_plugin_information<plugin_version>(args...),               // no default, must be defined
        .f_library_version =        find_plugin_information<plugin_library_version>(args...),       // no default, must be defined
        .f_last_modification =      find_plugin_information<plugin_last_modification>(args...),     // no default, must be defined
        .f_name =                   find_plugin_information<plugin_name>(args...),                  // no default, must be defined
        .f_description =            find_plugin_information<plugin_description>(args..., plugin_description()),
        .f_help_uri =               find_plugin_information<plugin_help_uri>(args..., plugin_help_uri()),
        .f_icon =                   find_plugin_information<plugin_icon>(args..., plugin_icon()),
        .f_categorization_tags =    find_plugin_set<plugin_categorization_tag>(args...),
        .f_dependencies =           find_plugin_set<plugin_dependency>(args...),
        .f_conflicts =              find_plugin_set<plugin_conflict>(args...),
        .f_suggestions =            find_plugin_set<plugin_suggestion>(args...),
        .f_settings_path =          find_plugin_information<plugin_settings_path>(args..., plugin_settings_path()),
    };
#pragma GCC diagnostic pop

    return def;
}






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


class plugin_names
{
public:
    typedef std::string                     name_t;
    typedef std::string                     filename_t;
    typedef std::map<name_t, filename_t>    names_t;

                                        plugin_names(plugin_paths const & paths, bool script_names = false);

    bool                                validate(name_t const & name);
    bool                                is_emcascript_reserved(std::string const & word);

    filename_t                          to_filename(name_t const & name);
    void                                push(name_t const & name);
    void                                add(std::string const & set);
    names_t                             names() const;

    void                                find_plugins(name_t const & prefix = name_t(), name_t const & suffix = name_t());

private:
    plugin_paths const                  f_paths;
    bool const                          f_prevent_script_names = false;
    names_t                             f_names = names_t();
};





class plugin;


class plugin_factory
{
public:
                                    plugin_factory(plugin_definition const & definition, std::shared_ptr<plugin> instance);
                                    ~plugin_factory();
                                    plugin_factory(plugin_factory const &) = delete;
    plugin_factory &                operator = (plugin_factory const &) = delete;

    plugin_definition const &       definition() const;
    std::shared_ptr<plugin>         instance() const;
    void                            register_plugin(char const * name, std::shared_ptr<plugin> p);

protected:
    void                            save_factory_in_plugin(plugin * p);

private:
    plugin_definition const &       f_definition;
    std::shared_ptr<plugin>         f_plugin = std::shared_ptr<plugin>();
};


class plugin_collection;


class plugin
{
public:
    typedef std::shared_ptr<plugin>     pointer_t;
    typedef std::vector<pointer_t>      vector_t;       // sorted by dependencies & then by name
    typedef std::map<std::string, pointer_t>
                                        map_t;          // sorted by name

                                        plugin();       // for the server
                                        plugin(plugin_factory const & factory);
                                        plugin(plugin const &) = delete;
    virtual                             ~plugin();
    plugin &                            operator = (plugin const &) = delete;

    version_t                           version() const;
    time_t                              last_modification() const;
    plugin_names::name_t                name() const;
    plugin_names::filename_t            filename() const;
    std::string                         description() const;
    std::string                         help_uri() const;
    std::string                         icon() const;
    string_set_t                        categorization_tags() const;
    string_set_t                        dependencies() const;
    string_set_t                        conflicts() const;
    string_set_t                        suggestions() const;
    std::string                         settings_path() const;
    plugin_collection *                 collection() const;

    virtual void                        bootstrap();
    virtual time_t                      do_update(time_t last_updated);

private:
    friend class detail::plugin_repository;
    friend class plugin_collection;
    friend class plugin_factory;

    plugin_factory const *              f_factory = nullptr;
    plugin_names::filename_t            f_filename = std::string();
    plugin_collection *                 f_collection = nullptr;
};


#define CPPTHREAD_PLUGIN_DEFAULTS(name) \
    typedef std::shared_ptr<name> pointer_t; \
    name(::cppthread::plugin_factory const & factory); \
    name(name const &) = delete; \
    virtual ~name(); \
    name & operator = (name const &) = delete; \
    static pointer_t instance()



class server
    : public plugin
{
public:
    // Do not use the CPPTHREAD_PLUGIN_DEFAULTS() because at this point
    // we do not have access to the factory
    //
    typedef std::shared_ptr<server> pointer_t;

                            server();
                            server(server const &) = delete;
    virtual                 ~server();
    server &                operator = (server const &) = delete;
    static pointer_t        instance();
};




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
std::cerr << "get_Server() has " << static_cast<void *>(f_server.get()) << "\n";
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




#define CPPTHREAD_PLUGIN_START(name, major, minor) \
    ::cppthread::plugin_definition const g_plugin_##name##_definition = ::cppthread::define_plugin( \
          ::cppthread::plugin_version(::cppthread::version_t(major, minor, 0)) \
        , ::cppthread::plugin_library_version(::cppthread::version_t(CPPTHREAD_VERSION_MAJOR, CPPTHREAD_VERSION_MINOR, CPPTHREAD_VERSION_PATCH)) \
        , ::cppthread::plugin_last_modification(UTC_BUILD_TIME_STAMP) \
        , ::cppthread::plugin_name(#name)


#define CPPTHREAD_PLUGIN_END(name) \
    ); \
    class plugin_##name##_factory : public ::cppthread::plugin_factory { \
    public: plugin_##name##_factory() \
        : plugin_factory(g_plugin_##name##_definition, std::make_shared<name>(*this)) \
        { register_plugin(#name, instance()); } \
    plugin_##name##_factory(plugin_##name##_factory const &) = delete; \
    plugin_##name##_factory & operator = (plugin_##name##_factory const &) = delete; \
    } g_plugin_##name##_factory; \
    name::name(::cppthread::plugin_factory const & factory) : plugin(factory) {} \
    name::~name() {} \
    name::pointer_t name::instance() { return std::static_pointer_cast<name>(g_plugin_##name##_factory.instance()); }


/** \brief Conditionally listen to a signal.
 *
 * This function checks whether a given plugin was loaded and if so
 * listen to one of its signals.
 *
 * The macro accepts the name of the listener plugin (it must be
 * 'this'), the name of the emitter plugin, and the name of the
 * signal to listen to.
 *
 * The listener must have a function named on_\<name of signal>.
 * The emitter is expected to define the signal using the
 * SNAP_SIGNAL() macro so the signal is called
 * signal_listen_\<name of signal>.
 *
 * \param[in] name  The name of the plugin connecting.
 * \param[in] emitter_name  The name of the plugin emitting this signal.
 * \param[in] emitter_class  The class with qualifiers if necessary of the plugin emitting this signal.
 * \param[in] signal  The name of the signal to listen to.
 * \param[in] args  The list of arguments to that signal.
 */
#define CPPTHREAD_PLUGIN_LISTEN(name, emitter_name, emitter_class, signal, args...) \
    if(::cppthread::plugins::exists(emitter_name)) \
        emitter_class::instance()->signal_listen_##signal( \
                    boost::bind(&name::on_##signal, this, ##args));

#define CPPTHREAD_PLUGIN_LISTEN0(name, emitter_name, emitter_class, signal) \
    if(::cpphtread::plugins::exists(emitter_name)) \
        emitter_class::instance()->signal_listen_##signal( \
                    boost::bind(&name::on_##signal, this));

/** \brief Compute the number of days in the month of February.
 * \private
 *
 * The month of February is used to adjust the date by 1 day over leap
 * years. Years are leap years when multiple of 4, but not if multiple
 * of 100, except if it is also a multiple of 400.
 *
 * The computation of a leap year is documented on Wikipedia:
 * http://www.wikipedia.org/wiki/Leap_year
 *
 * \param[in] year  The year of the date to convert.
 *
 * \return 28 or 29 depending on whether the year is a leap year
 */
#define _CPPTHREAD_PLUGIN_UNIX_TIMESTAMP_FDAY(year) \
    (((year) % 400) == 0 ? 29LL : \
        (((year) % 100) == 0 ? 28LL : \
            (((year) % 4) == 0 ? 29LL : \
                28LL)))

/** \brief Compute the number of days in a year.
 * \private
 *
 * This macro returns the number of day from the beginning of the
 * year the (year, month, day) value represents.
 *
 * \param[in] year  The 4 digits year concerned.
 * \param[in] month  The month (1 to 12).
 * \param[in] day  The day of the month (1 to 31)
 *
 * \return The number of days within that year (starting at 1)
 */
#define _CPPTHREAD_PLUGIN_UNIX_TIMESTAMP_YDAY(year, month, day) \
    ( \
        /* January */    static_cast<qint64>(day) \
        /* February */ + ((month) >=  2 ? 31LL : 0LL) \
        /* March */    + ((month) >=  3 ? _CPPTHREAD_PLUGIN_UNIX_TIMESTAMP_FDAY(year) : 0LL) \
        /* April */    + ((month) >=  4 ? 31LL : 0LL) \
        /* May */      + ((month) >=  5 ? 30LL : 0LL) \
        /* June */     + ((month) >=  6 ? 31LL : 0LL) \
        /* July */     + ((month) >=  7 ? 30LL : 0LL) \
        /* August */   + ((month) >=  8 ? 31LL : 0LL) \
        /* September */+ ((month) >=  9 ? 31LL : 0LL) \
        /* October */  + ((month) >= 10 ? 30LL : 0LL) \
        /* November */ + ((month) >= 11 ? 31LL : 0LL) \
        /* December */ + ((month) >= 12 ? 30LL : 0LL) \
    )

/** \brief Compute a Unix date from a hard coded date.
 *
 * This macro is used to compute a Unix date from a date defined as 6 numbers:
 * year, month, day, hour, minute, second. Each number is expected to be an
 * integer although it could very well be an expression. The computation takes
 * the year and month in account to compute the year day which is used by
 * the do_update() functions.
 *
 * The year is expected to be written as a 4 digit number (1998, 2012, etc.)
 *
 * Each number is expected to represent a valid date. If a number is out of
 * range, then the date is still computed. It will just represent a valid
 * date, just not exactly what you wrote down.
 *
 * The math used in this macro comes from a FreeBSD implementation of mktime
 * (http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap04.html#tag_04_15)
 *
 * \code
 * tm_sec + tm_min*60 + tm_hour*3600 + tm_yday*86400 +
 *   (tm_year-70)*31536000 + ((tm_year-69)/4)*86400 -
 *   ((tm_year-1)/100)*86400 + ((tm_year+299)/400)*86400
 * \endcode
 *
 * Note that we expect the year to be 1970 and not 0, 2000 and not 30, etc.
 * For this reason our macro subtract values from the year that are different
 * from those shown in the FreeBSD sample code.
 *
 * Also the month must be a number from 1 to 12 and not 0 to 11 as used
 * in various Unix structures.
 *
 * \param[in] year  The year representing this Unix timestamp.
 * \param[in] month  The month representing this Unix timestamp.
 * \param[in] day  The day representing this Unix timestamp.
 * \param[in] hour  The hour representing this Unix timestamp.
 * \param[in] minute  The minute representing this Unix timestamp.
 * \param[in] second  The second representing this Unix timestamp.
 */
#define CPPTHREAD_PLUGIN_UNIX_TIMESTAMP(year, month, day, hour, minute, second) \
    ( /* time */ static_cast<qint64>(second) \
                + static_cast<qint64>(minute) * 60LL \
                + static_cast<qint64>(hour) * 3600LL \
    + /* year day (month + day) */ (_CPPTHREAD_PLUGIN_UNIX_TIMESTAMP_YDAY(year, month, day) - 1) * 86400LL \
    + /* year */ (static_cast<qint64>(year) - 1970LL) * 31536000LL \
                + ((static_cast<qint64>(year) - 1969LL) / 4LL) * 86400LL \
                - ((static_cast<qint64>(year) - 1901LL) / 100LL) * 86400LL \
                + ((static_cast<qint64>(year) - 1601LL) / 400LL) * 86400LL )

/** \brief Initialize the do_update() function.
 *
 * This macro is used to initialize the do_update() function by creating a
 * variable that is going to be given a date.
 */
#define CPPTHREAD_PLUGIN_PLUGIN_UPDATE_INIT() int64_t last_plugin_update(CPPTHREAD_PLUGIN_UNIX_TIMESTAMP(1990, 1, 1, 0, 0, 0) * 1000000LL);

/** \brief Create an update entry in your on_update() signal implementation.
 *
 * This macro is used to generate the necessary code to test the latest
 * update date and the date of the specified update.
 *
 * The function is called if the last time the website was updated it
 * was before this update. The function is then called with its own
 * date in micro-seconds (usec).
 *
 * \warning
 * The parameter to the on_update() function must be named last_updated for
 * this macro to compile as expected.
 *
 * \param[in] year  The year representing this Unix timestamp.
 * \param[in] month  The month representing this Unix timestamp.
 * \param[in] day  The day representing this Unix timestamp.
 * \param[in] hour  The hour representing this Unix timestamp.
 * \param[in] minute  The minute representing this Unix timestamp.
 * \param[in] second  The second representing this Unix timestamp.
 * \param[in] function  The name of the function to call if the update is required.
 */
#define CPPTHREAD_PLUGIN_PLUGIN_UPDATE(year, month, day, hour, minute, second, function) \
    if(last_plugin_update > CPPTHREAD_PLUGIN_UNIX_TIMESTAMP(year, month, day, hour, minute, second) * 1000000LL) { \
        throw plugins::plugin_exception_invalid_order("the updates in your do_update() functions must appear in increasing order in regard to date and time"); \
    } \
    last_plugin_update = CPPTHREAD_PLUGIN_UNIX_TIMESTAMP(year, month, day, hour, minute, second) * 1000000LL; \
    if(last_updated < last_plugin_update) { \
        function(last_plugin_update); \
    }

/** \brief End the plugin update function.
 *
 * Use the macro as the very last line in your plugin do_update() function.
 * It returns the lastest update of your plugin.
 *
 * This function adds a return statement so anything appearing after it
 * will be ignored (not reached.)
 */
#define CPPTHREAD_PLUGIN_PLUGIN_UPDATE_EXIT() return last_plugin_update;




} // namespace cppthread
// vim: ts=4 sw=4 et
