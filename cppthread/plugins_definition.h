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
#include    "cppthread/plugins_utils.h"

//#include    "cppthread/exception.h"
#include    "cppthread/version.h"       // used in CPPTHREAD_PLUGIN_START()


// snapdev lib
//
#include    <snapdev/not_used.h>


// C++ set
//
//#include    <map>
//#include    <memory>
#include    <set>
//#include    <string>
//#include    <vector>


namespace cppthread
{



typedef std::set<std::string>           string_set_t;



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


// helper macros to define a plugin definition structure
//
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



} // namespace cppthread
// vim: ts=4 sw=4 et
