// Copyright (c) 2006-2022  Made to Order Software Corp.  All Rights Reserved
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
#include    "catch_main.h"

#include    "plugin_testme.h"


// cppthread lib
//
#include    <cppthread/plugins.h>


// snapdev lib
//
#include    <snapdev/not_reached.h>


// C++ lib
//
#include    <fstream>


// C lib
//
#include    <unistd.h>
#include    <sys/stat.h>
#include    <sys/types.h>


// last include
//
#include    <snapdev/poison.h>





CATCH_TEST_CASE("plugin_paths", "[plugins] [paths]")
{
    CATCH_START_SECTION("empty size/at when empty")
    {
        cppthread::plugin_paths p;

        CATCH_REQUIRE(p.size() == 0);

        for(int idx(-10); idx <= 10; ++idx)
        {
            CATCH_REQUIRE(p.at(idx) == std::string());
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("canonicalize empty path")
    {
        cppthread::plugin_paths p;

        CATCH_REQUIRE_FALSE(p.get_allow_redirects());
        CATCH_REQUIRE_THROWS_MATCHES(
                  p.canonicalize(cppthread::plugin_paths::path_t())
                , cppthread::cppthread_invalid_error
                , Catch::Matchers::ExceptionMessage(
                          "cppthread_exception: path cannot be an empty string."));

        p.set_allow_redirects(true);
        CATCH_REQUIRE(p.get_allow_redirects());
        CATCH_REQUIRE_THROWS_MATCHES(
                  p.canonicalize(cppthread::plugin_paths::path_t())
                , cppthread::cppthread_invalid_error
                , Catch::Matchers::ExceptionMessage(
                          "cppthread_exception: path cannot be an empty string."));

        p.set_allow_redirects(false);
        CATCH_REQUIRE_FALSE(p.get_allow_redirects());
        CATCH_REQUIRE_THROWS_MATCHES(
                  p.canonicalize(cppthread::plugin_paths::path_t())
                , cppthread::cppthread_invalid_error
                , Catch::Matchers::ExceptionMessage(
                          "cppthread_exception: path cannot be an empty string."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("canonicalize base root path")
    {
        for(int idx(1); idx <= 10; ++idx)
        {
            cppthread::plugin_paths p;

            cppthread::plugin_paths::path_t root(idx, '/');
            CATCH_REQUIRE(p.canonicalize(root) == "/");
            p.set_allow_redirects(true);
            CATCH_REQUIRE(p.canonicalize(root) == "/");
            p.set_allow_redirects(false);
            CATCH_REQUIRE(p.canonicalize(root) == "/");
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("canonicalize root path with one \"..\"")
    {
        std::vector<cppthread::plugin_paths::path_t> paths =
        {
            "this",
            "long",
            "..",
            "short",
            "root",
            "path",
        };

        for(cppthread::plugin_paths::path_t::size_type l(1); l < paths.size(); ++l)
        {
            cppthread::plugin_paths p;
            cppthread::plugin_paths::path_t expected;
            int count(rand() % 10 + 1);
            cppthread::plugin_paths::path_t path(count , '/');
            for(cppthread::plugin_paths::path_t::size_type c(0); c < l; ++c)
            {
                if(paths[c] != ".."
                && (c + 1 >= l || paths[c + 1] != ".."))
                {
                    expected += '/';
                    expected += paths[c];
                }

                path += paths[c];

                count = rand() % 10 + 1;
                cppthread::plugin_paths::path_t sep(count , '/');
                path += sep;
            }

//std::cerr << "+++ expected = [" << expected << "] and path [" << path << "]\n";

            // verify that we got expected as a canonicalize path
            //
            CATCH_CHECK(p.canonicalize(expected) == expected);
            CATCH_REQUIRE(p.canonicalize(path) == expected);

            p.set_allow_redirects(true);
            CATCH_CHECK(p.canonicalize(expected) == expected);
            CATCH_REQUIRE(p.canonicalize(path) == expected);

            p.set_allow_redirects(false);
            CATCH_CHECK(p.canonicalize(expected) == expected);
            CATCH_REQUIRE(p.canonicalize(path) == expected);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("canonicalize root path with too many \"..\"")
    {
        cppthread::plugin_paths p;

        p.set_allow_redirects(false);
        CATCH_REQUIRE(p.canonicalize("/this/long/../../../..//") == "/");
        p.set_allow_redirects(true);
        CATCH_REQUIRE(p.canonicalize("/this/long/../../../..//") == "/");

        p.set_allow_redirects(false);
        CATCH_REQUIRE(p.canonicalize("/this//long/../../../../root/home/path/") == "/root/home/path");
        p.set_allow_redirects(true);
        CATCH_REQUIRE(p.canonicalize("/this//long/../../../../root/home/path/") == "/root/home/path");
        p.set_allow_redirects(false);
        CATCH_REQUIRE(p.canonicalize("/this/long/../..//./../root/home/path/") == "/root/home/path");
        p.set_allow_redirects(true);
        CATCH_REQUIRE(p.canonicalize("/this/long/../..//./../root/home/path/") == "/root/home/path");
        p.set_allow_redirects(false);
        CATCH_REQUIRE(p.canonicalize("/this/long/.././../../root//home/path/") == "/root/home/path");
        p.set_allow_redirects(true);
        CATCH_REQUIRE(p.canonicalize("/this/long/.././../../root//home/path/") == "/root/home/path");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("canonicalize relative path with \".\" and \"..\"")
    {
        cppthread::plugin_paths p;

        CATCH_REQUIRE(p.canonicalize("this/./relative/./angle/.././path//cleaned/up") == "this/relative/path/cleaned/up");
        p.set_allow_redirects(true);
        CATCH_REQUIRE(p.canonicalize("this/./relative/./angle/.././path//cleaned/up") == "this/relative/path/cleaned/up");
        p.set_allow_redirects(false);
        CATCH_REQUIRE(p.canonicalize("this/./relative/./angle/.././path//cleaned/up") == "this/relative/path/cleaned/up");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("canonicalize relative path with too many \"..\"")
    {
        cppthread::plugin_paths p;

        p.set_allow_redirects(true);
        CATCH_REQUIRE(p.canonicalize("this/long/../../../..//") == "../..");
        p.set_allow_redirects(false);
        CATCH_REQUIRE_THROWS_MATCHES(
                  p.canonicalize("this/long/../../../..//")
                , cppthread::cppthread_invalid_error
                , Catch::Matchers::ExceptionMessage(
                          "cppthread_exception: the path \"this/long/../../../..//\" going outside of the allowed range."));

        p.set_allow_redirects(true);
        CATCH_REQUIRE(p.canonicalize("this//long/../../../../root/home/path/") == "../../root/home/path");
        p.set_allow_redirects(false);
        CATCH_REQUIRE_THROWS_MATCHES(
                  p.canonicalize("this//long/../../../../root/home/path/")
                , cppthread::cppthread_invalid_error
                , Catch::Matchers::ExceptionMessage(
                          "cppthread_exception: the path \"this//long/../../../../root/home/path/\" going outside of the allowed range."));

        p.set_allow_redirects(true);
        CATCH_REQUIRE(p.canonicalize("this/long/..//./../../root/home/path/") == "../root/home/path");
        p.set_allow_redirects(false);
        CATCH_REQUIRE_THROWS_MATCHES(
                  p.canonicalize("this/long/..//./../../root/home/path/")
                , cppthread::cppthread_invalid_error
                , Catch::Matchers::ExceptionMessage(
                          "cppthread_exception: the path \"this/long/..//./../../root/home/path/\" going outside of the allowed range."));

        p.set_allow_redirects(true);
        CATCH_REQUIRE(p.canonicalize("this/long/../.././../root//home//path//") == "../root/home/path");
        p.set_allow_redirects(false);
        CATCH_REQUIRE_THROWS_MATCHES(
                  p.canonicalize("this/long/../.././../root//home//path//")
                , cppthread::cppthread_invalid_error
                , Catch::Matchers::ExceptionMessage(
                          "cppthread_exception: the path \"this/long/../.././../root//home//path//\" going outside of the allowed range."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("push the same paths")
    {
        cppthread::plugin_paths p;

        p.push("path/one");
        p.push("path/two");
        p.push("path/three");
        p.push("path/two");
        p.push("path/one");

        CATCH_REQUIRE(p.size() == 3);
        CATCH_REQUIRE(p.at(0) == "path/one");
        CATCH_REQUIRE(p.at(1) == "path/two");
        CATCH_REQUIRE(p.at(2) == "path/three");
        CATCH_REQUIRE(p.at(3) == "");

        p.erase("path/four");

        CATCH_REQUIRE(p.size() == 3);
        CATCH_REQUIRE(p.at(0) == "path/one");
        CATCH_REQUIRE(p.at(1) == "path/two");
        CATCH_REQUIRE(p.at(2) == "path/three");
        CATCH_REQUIRE(p.at(3) == "");

        p.erase("path/two");

        CATCH_REQUIRE(p.size() == 2);
        CATCH_REQUIRE(p.at(0) == "path/one");
        CATCH_REQUIRE(p.at(1) == "path/three");
        CATCH_REQUIRE(p.at(2) == "");
        CATCH_REQUIRE(p.at(3) == "");

        p.erase("path/one");

        CATCH_REQUIRE(p.size() == 1);
        CATCH_REQUIRE(p.at(0) == "path/three");
        CATCH_REQUIRE(p.at(1) == "");
        CATCH_REQUIRE(p.at(2) == "");
        CATCH_REQUIRE(p.at(3) == "");

        p.erase("path/three");

        CATCH_REQUIRE(p.size() == 0);
        CATCH_REQUIRE(p.at(0) == "");
        CATCH_REQUIRE(p.at(1) == "");
        CATCH_REQUIRE(p.at(2) == "");
        CATCH_REQUIRE(p.at(3) == "");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("push invalid path")
    {
        cppthread::plugin_paths p;

        CATCH_REQUIRE_THROWS_MATCHES(
                  p.canonicalize("this/long/../.././../root//home//path//")
                , cppthread::cppthread_invalid_error
                , Catch::Matchers::ExceptionMessage(
                          "cppthread_exception: the path \"this/long/../.././../root//home//path//\" going outside of the allowed range."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("add paths")
    {
        cppthread::plugin_paths p;

        p.set_allow_redirects(true);
        p.add("this/long/../../../..//"
             ":this//long/../../../../root/home/path/"
             ":this/long/..//./../../root/home/path/"
             ":this/long/../.././..//home/user/path//");
        CATCH_REQUIRE(p.size() == 4);
        CATCH_REQUIRE(p.at(0) == "../..");
        CATCH_REQUIRE(p.at(1) == "../../root/home/path");
        CATCH_REQUIRE(p.at(2) == "../root/home/path");
        CATCH_REQUIRE(p.at(3) == "../home/user/path");
    }
    CATCH_END_SECTION()
}



CATCH_TEST_CASE("plugin_names", "[plugins] [names]")
{
    CATCH_START_SECTION("empty by default")
    {
        cppthread::plugin_paths p;
        p.add("/usr/local/lib/snaplogger/plugins:/usr/lib/snaplogger/plugins");

        cppthread::plugin_names n(p);

        CATCH_REQUIRE(n.names().empty());
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("validate names")
    {
        cppthread::plugin_paths p;
        p.add("/usr/local/lib/snaplogger/plugins:/usr/lib/snaplogger/plugins");

        cppthread::plugin_names n(p);

        CATCH_CHECK(n.validate("_"));
        CATCH_CHECK(n.validate("_valid"));
        CATCH_CHECK(n.validate("_identifier6"));
        CATCH_CHECK(n.validate("_9"));

        CATCH_CHECK_FALSE(n.validate(""));
        CATCH_CHECK_FALSE(n.validate(cppthread::plugin_names::name_t()));
        CATCH_CHECK_FALSE(n.validate("0"));
        CATCH_CHECK_FALSE(n.validate("9_"));
        CATCH_CHECK_FALSE(n.validate("dotted.word"));
        CATCH_CHECK_FALSE(n.validate(".dot"));
        CATCH_CHECK_FALSE(n.validate("dashed-word"));
        CATCH_CHECK_FALSE(n.validate("-dash"));

        for(int c('a'); c <= 'z'; ++c)
        {
            std::string word;
            word += c;
            CATCH_CHECK(n.validate(word));
        }

        for(int c('A'); c <= 'Z'; ++c)
        {
            std::string word;
            word += c;
            CATCH_CHECK(n.validate(word));
        }

        for(int c('0'); c <= '9'; ++c)
        {
            std::string word;
            word += '_';
            word += c;
            CATCH_CHECK(n.validate(word));
        }

        for(int c(1); c <= 0x7F; ++c)
        {
            if(c == '_'
            || (c >= 'a' && c <= 'z')
            || (c >= 'A' && c <= 'Z')
            || (c >= '0' && c <= '9'))
            {
                continue;
            }
            std::string word;
            word += '_';
            word += c;
            CATCH_CHECK_FALSE(n.validate(word));
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("validate non-script names")
    {
        cppthread::plugin_paths p;
        p.add("/usr/local/lib/snaplogger/plugins:/usr/lib/snaplogger/plugins");

        cppthread::plugin_names n(p, true);

        CATCH_CHECK(n.validate("_"));
        CATCH_CHECK(n.validate("_valid"));
        CATCH_CHECK(n.validate("_identifier6"));
        CATCH_CHECK(n.validate("_9"));

        CATCH_CHECK_FALSE(n.validate(""));
        CATCH_CHECK_FALSE(n.validate(cppthread::plugin_names::name_t()));
        CATCH_CHECK_FALSE(n.validate("0"));
        CATCH_CHECK_FALSE(n.validate("9_"));
        CATCH_CHECK_FALSE(n.validate("dotted.word"));
        CATCH_CHECK_FALSE(n.validate(".dot"));
        CATCH_CHECK_FALSE(n.validate("dashed-word"));
        CATCH_CHECK_FALSE(n.validate("-dash"));

        for(int c('a'); c <= 'z'; ++c)
        {
            std::string word;
            word += c;
            CATCH_CHECK(n.validate(word));
        }

        for(int c('A'); c <= 'Z'; ++c)
        {
            std::string word;
            word += c;
            CATCH_CHECK(n.validate(word));
        }

        for(int c('0'); c <= '9'; ++c)
        {
            std::string word;
            word += '_';
            word += c;
            CATCH_CHECK(n.validate(word));
        }

        for(int c(1); c <= 0x7F; ++c)
        {
            if(c == '_'
            || (c >= 'a' && c <= 'z')
            || (c >= 'A' && c <= 'Z')
            || (c >= '0' && c <= '9'))
            {
                continue;
            }
            std::string word;
            word += '_';
            word += c;
            CATCH_CHECK_FALSE(n.validate(word));
        }

        std::vector<std::string> const reserved_keywords = {
              "await"
            , "break"
            , "case"
            , "catch"
            , "class"
            , "const"
            , "continue"
            , "debugger"
            , "default"
            , "delete"
            , "do"
            , "else"
            , "enum"
            , "export"
            , "extends"
            , "false"
            , "finally"
            , "for"
            , "function"
            , "if"
            , "import"
            , "in"
            , "instanceof"
            , "new"
            , "null"
            , "return"
            , "super"
            , "switch"
            , "this"
            , "throw"
            , "true"
            , "try"
            , "typeof"
            , "var"
            , "void"
            , "while"
            , "with"
            , "yield"
        };
        for(auto k : reserved_keywords)
        {
            CATCH_CHECK_FALSE(n.validate(k));
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("add one unknown and one known name")
    {
        cppthread::plugin_paths p;
        p.add(CMAKE_BINARY_DIR "/tests:/usr/local/lib/snaplogger/plugins:/usr/lib/snaplogger/plugins");

        cppthread::plugin_names n(p, true);

        // test with an obviously unexistant plugin
        //
        cppthread::plugin_names::filename_t filename(n.to_filename("unknown"));
        CATCH_REQUIRE(filename.empty());

        // test with the real thing
        //
        filename = n.to_filename("testme");
        CATCH_REQUIRE(filename == CMAKE_BINARY_DIR "/tests/libtestme.so");

        // the following creates fake plugins so we can test all possible
        // cases (there are 4 of them)
        //
        {       // <name>.so
            std::string fake(CMAKE_BINARY_DIR "/tests/fake.so");
            unlink(fake.c_str());
            {
                std::ofstream out(fake);
                out << "fake plugin 1\n";
            }
            filename = n.to_filename("fake");
            CATCH_REQUIRE(filename.empty());
            chmod(fake.c_str(), 0755);
            filename = n.to_filename("fake");
            CATCH_REQUIRE(filename == fake);
            unlink(fake.c_str());
        }
        {       // lib<name>.so
            std::string fake(CMAKE_BINARY_DIR "/tests/libfake.so");
            unlink(fake.c_str());
            {
                std::ofstream out(fake);
                out << "fake plugin 2\n";
            }
            filename = n.to_filename("fake");
            CATCH_REQUIRE(filename.empty());
            chmod(fake.c_str(), 0755);
            filename = n.to_filename("fake");
            CATCH_REQUIRE(filename == fake);
            unlink(fake.c_str());
        }
        {       // <name>/<name>.so
            std::string subdir(CMAKE_BINARY_DIR "/tests/fake");
            mkdir(subdir.c_str(), 0750);
            {
                std::string fake(CMAKE_BINARY_DIR "/tests/fake/fake.so");
                unlink(fake.c_str());
                {
                    std::ofstream out(fake);
                    out << "fake plugin 3\n";
                }
                filename = n.to_filename("fake");
                CATCH_REQUIRE(filename.empty());
                chmod(fake.c_str(), 0755);
                filename = n.to_filename("fake");
                CATCH_REQUIRE(filename == fake);
                unlink(fake.c_str());
            }
            {
                std::string fake(CMAKE_BINARY_DIR "/tests/fake/libfake.so");
                unlink(fake.c_str());
                {
                    std::ofstream out(fake);
                    out << "fake plugin 4\n";
                }
                filename = n.to_filename("fake");
                CATCH_REQUIRE(filename.empty());
                chmod(fake.c_str(), 0755);
                filename = n.to_filename("fake");
                CATCH_REQUIRE(filename == fake);
                unlink(fake.c_str());
            }
            rmdir(subdir.c_str());
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("test filename without any paths")
    {
        cppthread::plugin_paths p;
        cppthread::plugin_names n(p);

        // test with an obviously unexistant plugin
        //
        cppthread::plugin_names::filename_t filename(n.to_filename("unknown"));
        CATCH_REQUIRE(filename.empty());

        // test with the real thing
        //
        filename = n.to_filename("testme");
        CATCH_REQUIRE(filename.empty());

        // the following creates fake plugins so we can test all possible
        // cases (there are 4 of them)
        //
        {       // <name>.so
            std::string fake("fake.so");
            unlink(fake.c_str());
            {
                std::ofstream out(fake);
                out << "fake plugin 1\n";
            }
            filename = n.to_filename("fake");
            CATCH_REQUIRE(filename.empty());
            chmod(fake.c_str(), 0755);
            filename = n.to_filename("fake");
            CATCH_REQUIRE(filename == "./" + fake);
            unlink(fake.c_str());
        }
        {       // lib<name>.so
            std::string fake("libfake.so");
            unlink(fake.c_str());
            {
                std::ofstream out(fake);
                out << "fake plugin 2\n";
            }
            filename = n.to_filename("fake");
            CATCH_REQUIRE(filename.empty());
            chmod(fake.c_str(), 0755);
            filename = n.to_filename("fake");
            CATCH_REQUIRE(filename == "./" + fake);
            unlink(fake.c_str());
        }
        {       // <name>/<name>.so
            std::string subdir("fake");
            mkdir(subdir.c_str(), 0750);
            {
                std::string fake("fake/fake.so");
                unlink(fake.c_str());
                {
                    std::ofstream out(fake);
                    out << "fake plugin 3\n";
                }
                filename = n.to_filename("fake");
                CATCH_REQUIRE(filename.empty());
                chmod(fake.c_str(), 0755);
                filename = n.to_filename("fake");
                CATCH_REQUIRE(filename == "./" + fake);
                unlink(fake.c_str());
            }
            {
                std::string fake("fake/libfake.so");
                unlink(fake.c_str());
                {
                    std::ofstream out(fake);
                    out << "fake plugin 4\n";
                }
                filename = n.to_filename("fake");
                CATCH_REQUIRE(filename.empty());
                chmod(fake.c_str(), 0755);
                filename = n.to_filename("fake");
                CATCH_REQUIRE(filename == "./" + fake);
                unlink(fake.c_str());
            }
            rmdir(subdir.c_str());
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("add a new names by hand (push)")
    {
        cppthread::plugin_paths p;
        p.add(CMAKE_BINARY_DIR "/tests:/usr/local/lib/snaplogger/plugins:/usr/lib/snaplogger/plugins");
        cppthread::plugin_names n(p);

        // test with an obviously unexistant plugin
        //
        n.push("testme");
        CATCH_REQUIRE(n.names().size() == 1);
        CATCH_REQUIRE(n.names().begin()->second == CMAKE_BINARY_DIR "/tests/libtestme.so");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("add a new names by hand (add)")
    {
        cppthread::plugin_paths p;
        p.add(CMAKE_BINARY_DIR "/tests:/usr/local/lib/snaplogger/plugins:/usr/lib/snaplogger/plugins");
        cppthread::plugin_names n(p);

        // TODO: look into having multiple plugins because that would allow
        //       us to test ':'
        //
        n.add("testme");
        CATCH_REQUIRE(n.names().size() == 1);
        CATCH_REQUIRE(n.names().begin()->second == CMAKE_BINARY_DIR "/tests/libtestme.so");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("add a names through the find_plugins() function")
    {
        cppthread::plugin_paths p;
        p.add(CMAKE_BINARY_DIR "/tests:/usr/local/lib/snaplogger/plugins:/usr/lib/snaplogger/plugins");
        cppthread::plugin_names n(p);
        n.find_plugins();
        CATCH_REQUIRE(n.names().size() == 1);
        CATCH_REQUIRE(n.names().begin()->second == CMAKE_BINARY_DIR "/tests/libtestme.so");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("add invalid names")
    {
        cppthread::plugin_paths p;
        p.add(CMAKE_BINARY_DIR "/tests:/usr/local/lib/snaplogger/plugins:/usr/lib/snaplogger/plugins");
        cppthread::plugin_names n(p);

        CATCH_REQUIRE_THROWS_MATCHES(
                  n.push("invalid-name")
                , cppthread::cppthread_invalid_error
                , Catch::Matchers::ExceptionMessage(
                          "cppthread_exception: invalid plugin name in \"invalid-name\"."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  n.push("non_existant")
                , cppthread::cppthread_not_found
                , Catch::Matchers::ExceptionMessage(
                          "cppthread_exception: plugin named \"non_existant\" not found in any of the specified paths."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  n.push("./libserver.so")
                , cppthread::cppthread_invalid_error
                , Catch::Matchers::ExceptionMessage(
                          "cppthread_exception: the name \"server\" is reserved for the main running process."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  n.push("./libjuju1.23.so")
                , cppthread::cppthread_invalid_error
                , Catch::Matchers::ExceptionMessage(
                          "cppthread_exception: invalid plugin name in \"juju1.23\" (from path \"./libjuju1.23.so\")."));
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("plugin_collection", "[plugins] [collection]")
{
    CATCH_START_SECTION("load the plugin")
    {
        cppthread::plugin_paths p;
        p.add(CMAKE_BINARY_DIR "/tests:/usr/local/lib/snaplogger/plugins:/usr/lib/snaplogger/plugins");
        cppthread::plugin_names n(p);
        n.find_plugins();
        cppthread::plugin_collection c(n);
        optional_namespace::data_t d;
        d.f_value = 0xA987;
        c.set_data(&d);
        CATCH_REQUIRE(c.load_plugins());
        optional_namespace::testme::pointer_t r(c.get_plugin_by_name<optional_namespace::testme>("testme"));
        CATCH_REQUIRE(r != nullptr);

        cppthread::version_t const v(r->version());
        CATCH_CHECK(v.f_major == 5);
        CATCH_CHECK(v.f_minor == 3);
        CATCH_CHECK(v.f_patch == 0);

        //time_t m(last_modification());

        CATCH_REQUIRE(r->name() == "testme");
        CATCH_REQUIRE(r->filename() == CMAKE_BINARY_DIR "/tests/libtestme.so");
        CATCH_REQUIRE(r->description() == "a test plugin to make sure it all works.");
        CATCH_REQUIRE(r->help_uri() == "https://snapwebsites.org/");
        CATCH_REQUIRE(r->icon() == "cute.ico");

        cppthread::string_set_t tags(r->categorization_tags());
        CATCH_REQUIRE(tags.size() == 3);
        CATCH_REQUIRE(tags.find("test") != tags.end());
        CATCH_REQUIRE(tags.find("powerful") != tags.end());
        CATCH_REQUIRE(tags.find("software") != tags.end());
        CATCH_REQUIRE(tags.find("undefined") == tags.end());

        // at this time we have a single plugins so not dependencies
        //
        cppthread::string_set_t dependencies(r->dependencies());
        CATCH_REQUIRE(dependencies.empty());

        cppthread::string_set_t conflicts(r->conflicts());
        CATCH_REQUIRE(conflicts.size() == 3);
        CATCH_REQUIRE(conflicts.find("other_test") != conflicts.end());
        CATCH_REQUIRE(conflicts.find("power_test") != conflicts.end());
        CATCH_REQUIRE(conflicts.find("unknown") != conflicts.end());
        CATCH_REQUIRE(conflicts.find("undefined") == conflicts.end());

        cppthread::string_set_t suggestions(r->suggestions());

        // TODO: this requires a signal...
        //std::string const msg(r->it_worked());
        //CATCH_CHECK(msg == "it worked, didn't it?");
    }
    CATCH_END_SECTION()
}




// vim: ts=4 sw=4 et
