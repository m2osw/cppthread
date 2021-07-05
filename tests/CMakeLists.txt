# Copyright (c) 2013-2021  Made to Order Software Corp.  All Rights Reserved
#
# https://snapwebsites.org/project/cppthread
# contact@m2osw.com
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

find_package(Catch2)

if(Catch2_FOUND)

    ##
    ## cppthread unittest
    ##
    project(unittest)

    add_definitions(
        -DCATCH_CONFIG_PREFIX_ALL
    )

    include_directories(
        ${CMAKE_BINARY_DIR}
        ${PROJECT_SOURCE_DIR}
        ${CATCH_INCLUDE_DIR}
        ${LIBEXCEPT_INCLUDE_DIRS}
        ${SNAPDEV_INCLUDE_DIRS}
    )

    add_executable(${PROJECT_NAME}
        catch_main.cpp

        catch_example.cpp
        catch_fifo.cpp
        catch_plugins.cpp
        catch_version.cpp
    )

    target_link_libraries(${PROJECT_NAME}
        cppthread
    )

    add_test(unittests ${PROJECT_NAME})

    ##
    ## Add a test plugin to verify the loading of it
    ##
    project(testme)

    add_library(${PROJECT_NAME} SHARED
        plugin_testme.cpp
    )

    target_link_libraries(${PROJECT_NAME}
        cppthread
    )

else(Catch2_FOUND)

    message("snapcatch2 not found... no test will be built.")

endif(Catch2_FOUND)

# vim: ts=4 sw=4 et
