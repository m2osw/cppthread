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
#pragma once

// self
//
#include    "cppthread/mutex.h"

#include    "cppthread/exception.h"


// snapdev lib
//
#include    <snapdev/not_used.h>


// C++ set
//
#include    <map>
#include    <memory>
#include    <set>
#include    <string>
#include    <vector>


namespace cppthread
{


typedef std::set<std::string>           string_set_t;


template<int N>
constexpr char const * validate_name(char const (&str)[N])
{
    static_assert(N > 0);

    static_assert(
               str[0] == '_'
            || (str[0] >= 'a' && str[0] <= 'z')
            || (str[0] >= 'A' && str[0] <= 'Z'));

    for(int i(1); i < N; ++i)
    {
        static_assert(
                   str[i] == '_'
                || (str[i] >= 'a' && str[i] <= 'z')
                || (str[i] >= 'A' && str[i] <= 'Z')
                || (str[i] >= '0' && str[i] <= '9'));
    }

    return &str;
}


constexpr time_t validate_date(time_t date)
{
    // any new plugin must be created after this date (about 2021/06/22 10:30)
    //
    if(date > 1624382757LL)
    {
        throw std::range_error("plugin dates are expected to be at least 2021/06/22 10:30");
    }
    return date;
}


template<typename T>
constexpr void validate_version(T const major, T const minor)
{
    if(major == 0 && minor == 0)
    {
        throw cppthread_logic_error("the plugin version cannot be 0.0.");
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
    time_t                              f_last_modification = 0;        // uses the compilation date & time converted to a Unix date
    std::string                         f_name = std::string();
    std::string                         f_description = std::string();
    std::string                         f_help_uri = std::string();
    std::string                         f_icon = std::string();
    string_set_t                        f_categorization_tags = string_set_t();
    string_set_t                        f_dependencies = string_set_t();
    string_set_t                        f_conflicts = string_set_t();
    string_set_t                        f_suggestions = string_set_t();
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
        : plugin_definition_value<char const *>(nullptr)
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
        : plugin_definition_value<char const *>(nullptr)
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
        : plugin_definition_value<char const *>(nullptr)
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
    //constexpr plugin_categorization_tag()
    //    : plugin_definition_value<char const *>()
    //{
    //}

    template<int N>
    constexpr plugin_categorization_tag(char const (&tag)[N])
        : plugin_definition_value<char const *>(validate_name(tag))
    {
    }
};


class plugin_dependencies
    : public plugin_definition_value<char const *>
{
public:
    constexpr plugin_dependencies()
        : plugin_definition_value<char const *>(nullptr)
    {
    }

    template<int N>
    constexpr plugin_dependencies(char const * (&dependencies)[N])
        : plugin_definition_value<char const *>(validate_name(dependencies))
    {
    }
};


class plugin_conflicts
    : public plugin_definition_value<char const *>
{
public:
    constexpr plugin_conflicts()
        : plugin_definition_value<char const *>(nullptr)
    {
    }

    template<int N>
    constexpr plugin_conflicts(char const * (&conflicts)[N])
        : plugin_definition_value<char const *>(validate_name(conflicts))
    {
    }
};


class plugin_suggestions
    : public plugin_definition_value<char const *>
{
public:
    constexpr plugin_suggestions()
        : plugin_definition_value<char const *>(nullptr)
    {
    }

    template<int N>
    constexpr plugin_suggestions(char const * (&suggestions)[N])
        : plugin_definition_value<char const *>(validate_name(suggestions))
    {
    }
};






template<typename T, typename F, class ...ARGS>
constexpr typename std::enable_if<std::is_same<T, F>::value, typename T::value_t>::type find_plugin_information(F first, ARGS ...args)
{
    snap::NOT_USED(args...);
    return first.get();
}


template<typename T, typename F, class ...ARGS>
constexpr typename std::enable_if<!std::is_same<T, F>::value, typename T::value_t>::type find_plugin_information(F first, ARGS ...args)
{
    snap::NOT_USED(first);
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
    snap::NOT_USED(first);
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
    snap::NOT_USED(first);
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
        .f_last_modification =      find_plugin_information<plugin_last_modification>(args...),     // no default, must be defined
        .f_name =                   find_plugin_information<plugin_name>(args...),                  // no default, must be defined
        .f_description =            find_plugin_information<plugin_description>(args..., plugin_description()),
        .f_help_uri =               find_plugin_information<plugin_help_uri>(args..., plugin_help_uri()),
        .f_icon =                   find_plugin_information<plugin_icon>(args..., plugin_icon()),
        .f_categorization_tags =    find_plugin_set<plugin_categorization_tag>(args...),
        .f_dependencies =           find_plugin_set<plugin_dependencies>(args...),
        .f_conflicts =              find_plugin_set<plugin_conflicts>(args...),
        .f_suggestions =            find_plugin_set<plugin_suggestions>(args...),
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
    path_t                              canonicalize_path(path_t const & path, bool allow_redirects = false);
    void                                push(path_t const & path);
    void                                add(std::string const & paths);
    void                                erase(std::string const & path);

private:
    paths_t                             f_paths = paths_t();
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
    names_t                             get_names() const;

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
    std::shared_ptr<plugin>         instance();
    void                            register_plugin(char const * name, std::shared_ptr<plugin> p);

private:
    plugin_definition const &       f_definition;
    std::shared_ptr<plugin>         f_plugin = std::shared_ptr<plugin>();
};




class plugin
{
public:
    typedef std::shared_ptr<plugin>     pointer_t;
    typedef std::vector<pointer_t>      vector_t;       // sorted by dependencies & then by name
    typedef std::map<std::string, pointer_t>
                                        map_t;          // sorted by name

                                        plugin(plugin_factory const & factory);
                                        plugin(plugin const &) = delete;
    virtual                             ~plugin();
    plugin &                            operator = (plugin const &) = delete;

    pointer_t                           instance();

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

    virtual void                        bootstrap(void * data);

private:
    plugin_factory const &              f_factory;
    plugin_names::filename_t            f_filename = std::string();
};


#define CPPTHREAD_PLUGIN_DEFAULTS(name) \
    public: \
    typedef std::shared_ptr<name> pointer_t; \
    name(); \
    virtual ~name(); \
    static pointer_t instance();
    



class plugin_collection
{
public:
    typedef std::shared_ptr<plugin_collection>  pointer_t;

                                        plugin_collection(plugin_names const & names);
                                        plugin_collection(plugin_collection const &) = delete;
    plugin_collection &                 operator = (plugin_collection const &) = delete;

    bool                                load_plugins();
    bool                                is_loaded(std::string const & name) const;

    plugin::pointer_t                   get_plugin_by_name(std::string const & name);

    void                                set_data(void * data);

private:
    mutex                               f_mutex = mutex();
    plugin_names const                  f_names;
    plugin::vector_t                    f_plugins = plugin::vector_t();             // unordered plugins
    plugin::vector_t                    f_ordered_plugins = plugin::vector_t();     // sorted plugins
    plugin::map_t                       f_plugins_by_name = plugin::map_t();        // plugins sorted by name only
    void *                              f_data = nullptr;
};




#define CPPTHREAD_PLUGIN_START(name, major, minor) \
    plugin_definition const g_plugin_##name##_definition = ::cppthread::define_plugin( \
          ::cppthread::plugin_version(version_t(major, minor, 0)) \
        , ::cppthread::plugin_library_version(version_t(CPPTHREAD_VERSION_MAJOR, CPPTHREAD_VERSION_MINOR, CPPTHREAD_VERSION_PATCH)) \
        , ::cppthread::plugin_last_modification(UTC_BUILD_TIME_STAMP) \
        , ::cppthread::name(#name)


#define CPPTHREAD_PLUGIN_END(name) \
    ); \
    class plugin_##name##_factory : public plugin_factory { \
    public: plugin_##name##_factory() \
        : plugin_factory(g_plugin_##name##_definition, std::make_shared<name>(*this)) \
        { register_plugin(#name, instance()); } \
    plugin_##name##_factory(plugin_##name##_factory const &) = delete; \
    plugin_##name##_factory & operator = (plugin_##name##_factory const &) = delete; \
    } g_plugin_##name##_factory; \
    name::name() : plugin(&g_plugin_##name##_factory) {} \
    name::~name() {} \
    name::pointer_t name::instance() { return std::static_pointer_cast<name>(g_plugin_##name##_factory::instance()); }




} // namespace cppthread
// vim: ts=4 sw=4 et
