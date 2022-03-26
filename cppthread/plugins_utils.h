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
// #include    "cppthread/mutex.h"

#include    "cppthread/exception.h"
// #include    "cppthread/version.h"       // used in CPPTHREAD_PLUGIN_START()
// 
// 
// // snapdev
// //
// #include    <snapdev/not_used.h>


// C++
//
// #include    <map>
// #include    <memory>
// #include    <set>
#include    <string>
// #include    <vector>


namespace cppthread
{



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
constexpr typename std::enable_if<std::is_signed<T>::value, bool>::type is_negative(T const value)
{
    return value < 0;
}


template<typename T>
constexpr typename std::enable_if<!std::is_signed<T>::value, bool>::type is_negative(T const)
{
    // unsigned type T can never be negative
    return false;
}


template<typename T>
constexpr void validate_version(T const major, T const minor, T const patch)
{
    if(major == 0 && minor == 0)
    {
        throw cppthread_logic_error("the plugin version cannot be 0.0.");
    }

    if(is_negative(major)
    || is_negative(minor)
    || is_negative(patch))
    {
        throw cppthread_logic_error("the plugin version cannot use negative numbers.");
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
                            validate_version(major, minor, patch);
                        }

    std::int32_t        f_major = 0;
    std::int32_t        f_minor = 0;
    std::int32_t        f_patch = 0;
};



} // namespace cppthread
// vim: ts=4 sw=4 et
