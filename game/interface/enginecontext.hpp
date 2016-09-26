/**
  *  \file game/interface/enginecontext.hpp
  */
#ifndef C2NG_GAME_INTERFACE_ENGINECONTEXT_HPP
#define C2NG_GAME_INTERFACE_ENGINECONTEXT_HPP

#include "interpreter/context.hpp"
#include "game/spec/shiplist.hpp"

namespace game { namespace interface {

    class EngineContext : public interpreter::Context {
     public:
        EngineContext(int nr, afl::base::Ptr<game::spec::ShipList> shipList);
        ~EngineContext();

        // Context:
        virtual bool lookup(const afl::data::NameQuery& name, PropertyIndex_t& result);
        virtual void set(PropertyIndex_t index, afl::data::Value* value);
        virtual afl::data::Value* get(PropertyIndex_t index);
        virtual bool next();
        virtual EngineContext* clone() const;
        virtual game::map::Object* getObject();
        virtual void enumProperties(interpreter::PropertyAcceptor& acceptor);

        // BaseValue:
        virtual String_t toString(bool readable) const;
        virtual void store(interpreter::TagNode& out, afl::io::DataSink& aux, afl::charset::Charset& cs, interpreter::SaveContext* ctx) const;

     private:
        int m_number;
        afl::base::Ptr<game::spec::ShipList> m_shipList;
    };

} }

#endif
