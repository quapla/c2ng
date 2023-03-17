/**
  *  \file u/t_game_ref_listobserver.cpp
  *  \brief Test for game::ref::ListObserver
  */

#include "game/ref/listobserver.hpp"

#include "t_game_ref.hpp"
#include "afl/io/nullfilesystem.hpp"
#include "afl/string/nulltranslator.hpp"
#include "game/game.hpp"
#include "game/map/planet.hpp"
#include "game/map/universe.hpp"
#include "game/ref/configuration.hpp"
#include "game/test/counter.hpp"
#include "game/test/root.hpp"
#include "game/turn.hpp"

namespace {
    struct Environment {
        afl::string::NullTranslator tx;
        afl::io::NullFileSystem fs;
        game::Session session;

        Environment()
            : tx(), fs(), session(tx, fs)
            { }
    };

    game::Root& addRoot(Environment& env)
    {
        if (env.session.getRoot().get() == 0) {
            env.session.setRoot(game::test::makeRoot(game::HostVersion()).asPtr());
        }
        return *env.session.getRoot();
    }

    game::Game& addGame(Environment& env)
    {
        if (env.session.getGame().get() == 0) {
            env.session.setGame(new game::Game());
        }
        return *env.session.getGame();
    }

    game::map::Planet& addPlanet(Environment& env, int nr, String_t name)
    {
        game::map::Planet& pl = *addGame(env).currentTurn().universe().planets().create(nr);
        pl.setPosition(game::map::Point(1000, 1000+nr));
        pl.setName(name);
        pl.internalCheck(addGame(env).mapConfiguration(), game::PlayerSet_t(1), 10, env.tx, env.session.log());
        return pl;
    }

    void addPlayer(Environment& env, int id, String_t name, String_t adj)
    {
        game::Player& pl = *addRoot(env).playerList().create(id);
        pl.setName(game::Player::ShortName, name);
        pl.setName(game::Player::AdjectiveName, adj);
    }
}

void
TestGameRefListObserver::testIt()
{
    Environment env;
    game::config::UserConfiguration& config = addRoot(env).userConfiguration();
    config[game::config::UserConfiguration::Sort_Ship].set(game::ref::ConfigSortByOwner);
    config[game::config::UserConfiguration::Sort_Ship_Secondary].set(game::ref::ConfigSortById);

    addPlanet(env, 1, "One").setOwner(3);
    addPlanet(env, 2, "Two").setOwner(5);
    addPlanet(env, 3, "Three").setOwner(5);
    addPlanet(env, 4, "Four").setOwner(3);
    addPlanet(env, 5, "Five").setOwner(3);

    addPlayer(env, 3, "The Birds", "bird");
    addPlayer(env, 5, "The Pirates", "pirate");

    game::ref::ListObserver testee;
    game::test::Counter ctr;
    testee.sig_listChange.add(&ctr, &game::test::Counter::increment);

    // Set session. This does not yet cause a change.
    TS_ASSERT_EQUALS(ctr.get(), 0);
    testee.setSession(env.session);
    testee.setConfigurationSelection(game::ref::REGULAR);
    TS_ASSERT_EQUALS(ctr.get(), 0);

    // Set list
    game::ref::List list;
    for (int i = 1; i <= 5; ++i) {
        list.add(game::Reference(game::Reference::Planet, i));
    }
    testee.setList(list);
    TS_ASSERT_EQUALS(ctr.get(), 1);

    // Verify result list
    //   0: == The Birds ==
    //   1: One
    //   2: Four
    //   3: Five
    //   4: == The Pirates ==
    //   5: Two
    //   6: Three
    {
        const game::ref::UserList& r = testee.getList();
        TS_ASSERT_EQUALS(r.size(), 7U);
        TS_ASSERT_EQUALS(r.get(0)->name, "The Birds");
        TS_ASSERT_EQUALS(r.get(1)->name, "Planet #1: One");
        TS_ASSERT_EQUALS(r.get(2)->name, "Planet #4: Four");
        TS_ASSERT_EQUALS(r.get(3)->name, "Planet #5: Five");
        TS_ASSERT_EQUALS(r.get(4)->name, "The Pirates");
        TS_ASSERT_EQUALS(r.get(5)->name, "Planet #2: Two");
        TS_ASSERT_EQUALS(r.get(6)->name, "Planet #3: Three");
    }

    TS_ASSERT_EQUALS(testee.getConfig().order.first, game::ref::ConfigSortByOwner);
    TS_ASSERT_EQUALS(testee.getConfig().order.second, game::ref::ConfigSortById);

    // Add extra; verify
    game::ref::UserList extra;
    extra.add(game::ref::UserList::OtherItem, "extra", game::Reference(), false, game::map::Object::NotPlayable, util::SkinColor::Red);
    testee.setExtra(extra);
    TS_ASSERT_EQUALS(ctr.get(), 2);

    // Verify result list
    {
        const game::ref::UserList& r = testee.getList();
        TS_ASSERT_EQUALS(r.size(), 9U);
        TS_ASSERT_EQUALS(r.get(6)->name, "Planet #3: Three");
        TS_ASSERT_EQUALS(r.get(7)->name, "Other");              // auto-inserted divider
        TS_ASSERT_EQUALS(r.get(8)->name, "extra");
    }

    // Change config; verify
    game::ref::Configuration newc;
    newc.order.first = game::ref::ConfigSortById;
    newc.order.second = game::ref::ConfigSortById;
    testee.setConfig(newc);
    TS_ASSERT_EQUALS(ctr.get(), 3);

    TS_ASSERT_EQUALS(config[game::config::UserConfiguration::Sort_Ship](), game::ref::ConfigSortById);

    // Verify result list
    //   1: One
    //   2: Two
    //   3: Three
    //   4: Four
    //   5: Five
    //   6: extra       // no divider automatically added
    {
        const game::ref::UserList& r = testee.getList();
        TS_ASSERT_EQUALS(r.size(), 6U);
        TS_ASSERT_EQUALS(r.get(0)->name, "Planet #1: One");
        TS_ASSERT_EQUALS(r.get(1)->name, "Planet #2: Two");
        TS_ASSERT_EQUALS(r.get(2)->name, "Planet #3: Three");
        TS_ASSERT_EQUALS(r.get(3)->name, "Planet #4: Four");
        TS_ASSERT_EQUALS(r.get(4)->name, "Planet #5: Five");
        TS_ASSERT_EQUALS(r.get(5)->name, "extra");
    }

    // Update content; verify
    env.session.getGame()->currentTurn().universe().planets().get(3)->setIsMarked(true);
    env.session.notifyListeners();
    TS_ASSERT_EQUALS(ctr.get(), 4);

    // Verify result list
    {
        const game::ref::UserList& r = testee.getList();
        TS_ASSERT_EQUALS(r.size(), 6U);
        TS_ASSERT_EQUALS(r.get(2)->marked, true);
    }
}

