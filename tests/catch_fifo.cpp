// Copyright (c) 2006-2024  Made to Order Software Corp.  All Rights Reserved
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

// cppthread lib
//
#include    <cppthread/fifo.h>

#include    <cppthread/exception.h>
#include    <cppthread/guard.h>
#include    <cppthread/item_with_predicate.h>
#include    <cppthread/life.h>
#include    <cppthread/mutex.h>
#include    <cppthread/pool.h>
#include    <cppthread/runner.h>
#include    <cppthread/thread.h>
#include    <cppthread/worker.h>


// self
//
#include    "catch_main.h"


// snapdev lib
//
#include    <snapdev/not_reached.h>


// C lib
//
#include    <unistd.h>



CATCH_TEST_CASE("fifo", "[fifo]")
{
    CATCH_START_SECTION("fifo: FIFO with constraints (own implementation)")
    {
        struct item_t
        {
            typedef std::shared_ptr<item_t>     pointer_t;

            bool valid_workload() const
            {
                return f_ready;
            }

            bool        f_ready = true;
            int         f_data = 0;
        };

        cppthread::fifo<item_t::pointer_t> msg;

        // no constraint, it comes out the way it went in
        //
        for(int count(0); count < 3; ++count)
        {
            item_t::pointer_t a(std::make_shared<item_t>());
            a->f_data = 1;
            msg.push_back(a);

            item_t::pointer_t b(std::make_shared<item_t>());
            b->f_data = 2;
            msg.push_back(b);

            item_t::pointer_t v;
            CATCH_REQUIRE(msg.pop_front(v, 0));
            CATCH_REQUIRE(v->f_data == 1);

            CATCH_REQUIRE(msg.pop_front(v, 0));
            CATCH_REQUIRE(v->f_data == 2);
        }

        // now add a constraint, it comes out reversed
        //
        for(int count(0); count < 3; ++count)
        {
            item_t::pointer_t a(std::make_shared<item_t>());
            a->f_data = 1;
            a->f_ready = false;
            msg.push_back(a);

            item_t::pointer_t b(std::make_shared<item_t>());
            b->f_data = 2;
            msg.push_back(b);

            item_t::pointer_t v;
            CATCH_REQUIRE(msg.pop_front(v, 0));
            CATCH_REQUIRE(v->f_data == 2);

            // remove the constraint
            //
            a->f_ready = true;

            CATCH_REQUIRE(msg.pop_front(v, 0));
            CATCH_REQUIRE(v->f_data == 1);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("fifo: FIFO with constraints (item_with_predicate implementation)")
    {
        struct item_t
            : public cppthread::item_with_predicate
        {
            typedef std::shared_ptr<item_t>     pointer_t;

            int         f_data = 0;
        };

        cppthread::fifo<item_t::pointer_t> msg;

        {
            item_t::pointer_t items[10];
            for(std::size_t i(0); i < sizeof(items) / sizeof(items[0]); ++i)
            {
                items[i] = std::make_shared<item_t>();
                items[i]->f_data = i + 1;
                msg.push_back(items[i]);
            }

            //items[0]->add_dependency(...); -- at least one item cannot have dependencies
            items[1]->add_dependency(items[0]);
            items[2]->add_dependency(items[1]);
            items[3]->add_dependency(items[0]);
            items[4]->add_dependencies({items[0], items[1], items[2], items[3]});
            items[5]->add_dependencies({items[3], items[4]});
            items[6]->add_dependency(items[2]);
            items[7]->add_dependency(items[6]);
            items[8]->add_dependency(items[6]);
            items[9]->add_dependencies({items[7], items[6]});

            for(std::size_t i(0); i < sizeof(items) / sizeof(items[0]); ++i)
            {
                // we do not want these anymore
                //
                items[i].reset();
            }

            for(std::size_t i(0); i < sizeof(items) / sizeof(items[0]); ++i)
            {
                item_t::pointer_t v;
                CATCH_REQUIRE(msg.pop_front(v, 0));

                // for this one, the order is simple
                //
                CATCH_REQUIRE(v->f_data == static_cast<int>(i + 1));
            }
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("fifo: FIFO with constraints -- Number 2 (item_with_predicate implementation)")
    {
        struct item_t
            : public cppthread::item_with_predicate
        {
            typedef std::shared_ptr<item_t>     pointer_t;

            int         f_data = 0;
        };

        cppthread::fifo<item_t::pointer_t> msg;

        {
            item_t::pointer_t items[10];
            for(std::size_t i(0); i < sizeof(items) / sizeof(items[0]); ++i)
            {
                items[i] = std::make_shared<item_t>();
                items[i]->f_data = i + 1;
                msg.push_back(items[i]);
            }

            // Complex set of dependencies
            //
            //       +-----------------+
            //       |                 |
            //       v                 |
            //    +----+   +----+      |     +----+
            //    |  1 |<--+  2 |<==== | ====+  7 |
            //    +--+-+   +----+      |     +----+
            //       |       ^         |       ^
            //       v       |         |       |
            //    +----+     |         |     +-+--+
            //    |  5 |     |         |     |  3 |
            //    +--+-+     |         |     +----+
            //       |       |         |       ^
            //       v       |         |       |
            //    +----+   +-+--+   +--+-+   +-+--+
            //    | 10 +-->|  4 |   |  6 +-->|  8 |
            //    +----+   +----+   +--+-+   +----+
            //       ^       ^         |
            //       |       |         |
            //       |     +-+--+      |
            //       +-----+  9 |<-----+
            //             +----+
            //
            items[0]->add_dependency(items[5]);
            items[1]->add_dependencies({items[0], items[3]});
            items[2]->add_dependency(items[7]);
            items[3]->add_dependencies({items[8], items[9]});
            items[4]->add_dependencies({items[6], items[0]});
            //items[5]->add_dependencies(...);
            items[6]->add_dependency(items[2]);
            items[7]->add_dependency(items[5]);
            items[8]->add_dependency(items[5]);
            items[9]->add_dependencies({items[8], items[4]});

            for(std::size_t i(0); i < sizeof(items) / sizeof(items[0]); ++i)
            {
                // we do not want these anymore
                //
                items[i].reset();
            }

            // The order is well known because we always try to pop the item
            // with the lowest number (i.e. first that was added to the FIFO)
            //
            {
                item_t::pointer_t v;

                CATCH_REQUIRE(msg.pop_front(v, 0));
                CATCH_REQUIRE(v->f_data == 6);

                // v needs to be reset otherwise the next pop "fails"
                //
                CATCH_REQUIRE_FALSE(msg.pop_front(v, 0));
            }

            {
                item_t::pointer_t v;
                CATCH_REQUIRE(msg.pop_front(v, 0));
                CATCH_REQUIRE(v->f_data == 1);
            }

            {
                item_t::pointer_t v;
                CATCH_REQUIRE(msg.pop_front(v, 0));
                CATCH_REQUIRE(v->f_data == 8);
            }

            {
                item_t::pointer_t v;
                CATCH_REQUIRE(msg.pop_front(v, 0));
                CATCH_REQUIRE(v->f_data == 3);
            }

            {
                item_t::pointer_t v;
                CATCH_REQUIRE(msg.pop_front(v, 0));
                CATCH_REQUIRE(v->f_data == 7);
            }

            {
                item_t::pointer_t v;
                CATCH_REQUIRE(msg.pop_front(v, 0));
                CATCH_REQUIRE(v->f_data == 5);
            }

            {
                item_t::pointer_t v;
                CATCH_REQUIRE(msg.pop_front(v, 0));
                CATCH_REQUIRE(v->f_data == 9);
            }

            {
                item_t::pointer_t v;
                CATCH_REQUIRE(msg.pop_front(v, 0));
                CATCH_REQUIRE(v->f_data == 10);
            }

            {
                item_t::pointer_t v;
                CATCH_REQUIRE(msg.pop_front(v, 0));
                CATCH_REQUIRE(v->f_data == 4);
            }

            {
                item_t::pointer_t v;
                CATCH_REQUIRE(msg.pop_front(v, 0));
                CATCH_REQUIRE(v->f_data == 2);
            }
        }
    }
    CATCH_END_SECTION()
}


// vim: ts=4 sw=4 et
