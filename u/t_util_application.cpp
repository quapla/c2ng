/**
  *  \file u/t_util_application.cpp
  *  \brief Test for util::Application
  */

#include "util/application.hpp"

#include "t_util.hpp"
#include "afl/io/internalstream.hpp"
#include "afl/io/nullfilesystem.hpp"
#include "afl/string/string.hpp"
#include "afl/sys/internalenvironment.hpp"

/** Test initialisation with an uncooperative environment.
    The uncooperative throws exceptions instead of attaching channels.
    Application initialisation must succeed anyway. */
void
TestUtilApplication::testInit()
{
    // Environment
    afl::sys::InternalEnvironment env;
    afl::io::NullFileSystem fs;

    // Application descendant
    class Tester : public util::Application {
     public:
        Tester(afl::sys::Environment& env, afl::io::FileSystem& fs)
            : Application(env, fs)
            { }
        virtual void appMain()
            {
                // Test all methods. Just verifies that they are callable without error.
                environment();
                fileSystem();
                translator();
                log();
                consoleLogger();
                standardOutput();
                errorOutput();

                // Test that we can write despite uncooperative environment.
                standardOutput().writeLine("hi");
            }
    };
    Tester t(env, fs);

    int n = t.run();
    TS_ASSERT_EQUALS(n, 0);
}

/** Interface test. */
void
TestUtilApplication::testExit()
{
    // Environment
    class FakeEnvironment : public afl::sys::InternalEnvironment {
     public:
        FakeEnvironment()
            : m_stream(*new afl::io::InternalStream())
            {
                setChannelStream(Output, m_stream.asPtr());
                setChannelStream(Error, m_stream.asPtr());
            }
        afl::base::ConstBytes_t getOutput()
            { return m_stream->getContent(); }
     private:
        afl::base::Ref<afl::io::InternalStream> m_stream;
    };

    // Regular exit
    {
        FakeEnvironment env;
        afl::io::NullFileSystem fs;

        class Tester : public util::Application {
         public:
            Tester(afl::sys::Environment& env, afl::io::FileSystem& fs)
                : Application(env, fs)
                { }
            virtual void appMain()
                { }
        };

        // Regular exit produces error 0
        TS_ASSERT_EQUALS(Tester(env, fs).run(), 0);

        // We didn't write anything, so output must be empty
        TS_ASSERT_EQUALS(env.getOutput().size(), 0U);
    }

    // Exit with error code
    {
        FakeEnvironment env;
        afl::io::NullFileSystem fs;

        class Tester : public util::Application {
         public:
            Tester(afl::sys::Environment& env, afl::io::FileSystem& fs)
                : Application(env, fs)
                { }
            virtual void appMain()
                { exit(42); }
        };
        TS_ASSERT_EQUALS(Tester(env, fs).run(), 42);
        TS_ASSERT_EQUALS(env.getOutput().size(), 0U);
    }

    // Exit with exception
    {
        FakeEnvironment env;
        afl::io::NullFileSystem fs;

        class Tester : public util::Application {
         public:
            Tester(afl::sys::Environment& env, afl::io::FileSystem& fs)
                : Application(env, fs)
                { }
            virtual void appMain()
                { throw std::runtime_error("hi mom"); }
        };
        TS_ASSERT_EQUALS(Tester(env, fs).run(), 1);
        TS_ASSERT_DIFFERS(env.getOutput().size(), 0U);
        TS_ASSERT_DIFFERS(afl::string::fromBytes(env.getOutput()).find("hi mom"), String_t::npos);
    }

    // Exit with nonstandard exception
    {
        FakeEnvironment env;
        afl::io::NullFileSystem fs;

        class Tester : public util::Application {
         public:
            Tester(afl::sys::Environment& env, afl::io::FileSystem& fs)
                : Application(env, fs)
                { }
            virtual void appMain()
                { throw "whatever"; }
        };
        TS_ASSERT_EQUALS(Tester(env, fs).run(), 1);
        TS_ASSERT_DIFFERS(env.getOutput().size(), 0U);
    }

    // Exit with errorExit
    {
        FakeEnvironment env;
        afl::io::NullFileSystem fs;

        class Tester : public util::Application {
         public:
            Tester(afl::sys::Environment& env, afl::io::FileSystem& fs)
                : Application(env, fs)
                { }
            virtual void appMain()
                { errorExit("broken"); }
        };
        TS_ASSERT_EQUALS(Tester(env, fs).run(), 1);
        TS_ASSERT_DIFFERS(env.getOutput().size(), 0U);
        TS_ASSERT_DIFFERS(afl::string::fromBytes(env.getOutput()).find("broken"), String_t::npos);
    }

    // Write partial line; must arrive completely.
    {
        FakeEnvironment env;
        afl::io::NullFileSystem fs;

        class Tester : public util::Application {
         public:
            Tester(afl::sys::Environment& env, afl::io::FileSystem& fs)
                : Application(env, fs)
                { }
            virtual void appMain()
                { standardOutput().writeText("ok"); }
        };
        TS_ASSERT_EQUALS(Tester(env, fs).run(), 0);
        TS_ASSERT_EQUALS(env.getOutput().size(), 2U);
        TS_ASSERT_SAME_DATA(env.getOutput().unsafeData(), "ok", 2);
    }
}

