# Copyright (c) 2013-2021 Made to Order Software Corp.
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

##
## cppthread library
##
project(cppthread)

# Put the version in the header file
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
)

add_library(${PROJECT_NAME} SHARED
    guard.cpp
    item_with_predicate.cpp
    life.cpp
    log.cpp
    mutex.cpp
    runner.cpp
    thread.cpp
    version.cpp
)

include_directories(
    ${LIBEXCEPT_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    ${LIBEXCEPT_LIBRARIES}
    pthread
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION
        ${CPPTHREAD_VERSION_MAJOR}.${CPPTHREAD_VERSION_MINOR}

    SOVERSION
        ${CPPTHREAD_VERSION_MAJOR}
)

install(
    TARGETS
        ${PROJECT_NAME}

    RUNTIME DESTINATION
        bin

    LIBRARY DESTINATION
        lib

    ARCHIVE DESTINATION
        lib
)

install(
    FILES
        exception.h
        fifo.h
        guard.h
        log.h
        mutex.h
        runner.h
        thread.h
        worker.h
        ${CMAKE_CURRENT_BINARY_DIR}/version.h

    DESTINATION
        include/cppthread
)


# vim: ts=4 sw=4 et
