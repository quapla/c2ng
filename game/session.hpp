/**
  *  \file game/session.hpp
  */
#ifndef C2NG_GAME_SESSION_HPP
#define C2NG_GAME_SESSION_HPP

#include <memory>
#include "interpreter/world.hpp"
#include "afl/sys/log.hpp"
#include "afl/base/ptr.hpp"
#include "game/spec/shiplist.hpp"
#include "game/interpreterinterface.hpp"
#include "afl/string/translator.hpp"
#include "afl/base/signalconnection.hpp"
#include "afl/io/stream.hpp"
#include "game/interface/userinterfacepropertyaccessor.hpp"
#include "interpreter/error.hpp"
#include "game/interface/userinterfacepropertystack.hpp"
#include "afl/io/filesystem.hpp"

namespace game {

    class Root;
    class Game;

    /** Session.
        This aggregates the dynamic parts of a session and controls lifetime of all child components.

        - It has a log node. Child objects use this log node, observers observe this log node.
        - It knows a Translator. Child objects use this translator.
        - It has an optional Root that can be reset or modified.
        - It has an optional ShipList that can be reset or modified.
        - It has an optional Game that can be reset or modified.
        - It has an interpreter::World. All interpreter objects live in that.
        - It provides in InterpreterInterface.

        Because all interpreter objects live in the embedded interpreter::World,
        it follows that no interpreter object can outlive a Session,
        but interpreter objects can outlive a Root, ShipList, or Game.
        Therefore,
        - interpreter objects can refer to a Session, World, InterpreterInterface, or Translator using a dumb reference.
        - interpreter objects must refer to Root, ShipList, or Game using a smart pointer.
        - interpreter objects must not be transferred between sessions.
        - Root, Session, Game, or Turn objects must not refer to each other. */
    class Session : private InterpreterInterface {
     public:
        explicit Session(afl::string::Translator& tx, afl::io::FileSystem& fs);

        ~Session();

        afl::sys::Log& log();
        void logError(const interpreter::Error& e);

        afl::string::Translator& translator();

        afl::base::Ptr<Root> getRoot();
        void setRoot(afl::base::Ptr<Root> root);

        afl::base::Ptr<game::spec::ShipList> getShipList();
        void setShipList(afl::base::Ptr<game::spec::ShipList> shipList);

        afl::base::Ptr<Game> getGame();
        void setGame(afl::base::Ptr<Game> game);

        game::interface::UserInterfacePropertyStack& uiPropertyStack();
        const game::interface::UserInterfacePropertyStack& uiPropertyStack() const;

        interpreter::World& world();

        InterpreterInterface& interface();

        util::RandomNumberGenerator& rng();

        bool executeFile(afl::io::Stream& file);

        void notifyListeners();

     private:
        afl::sys::Log m_log;
        afl::string::Translator& m_translator;
        afl::base::Ptr<Root> m_root;
        afl::base::Ptr<game::spec::ShipList> m_shipList;
        afl::base::Ptr<Game> m_game;
        game::interface::UserInterfacePropertyStack m_uiPropertyStack;
        interpreter::World m_world;
        util::RandomNumberGenerator m_rng;

        afl::base::SignalConnection conn_hostConfigToMap;
        afl::base::SignalConnection conn_userConfigToMap;

        // InterpreterInterface:
        virtual bool evaluate(Scope scope, int id, String_t expr);
        virtual String_t getComment(Scope scope, int id);
        virtual bool hasTask(Scope scope, int id);
        virtual bool getHullShortName(int nr, String_t& out);
        virtual bool getPlayerAdjective(int nr, String_t& out);

        void initWorld();

        // Signals:
        void connectSignals();
        void updateMap();
    };

}

#endif
