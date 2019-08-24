/**
  *  \file server/play/mainpacker.hpp
  */
#ifndef C2NG_SERVER_PLAY_MAINPACKER_HPP
#define C2NG_SERVER_PLAY_MAINPACKER_HPP

#include "server/play/packer.hpp"
#include "game/session.hpp"

namespace server { namespace play {

    class MainPacker : public Packer {
     public:
        MainPacker(game::Session& session, const std::map<String_t, String_t>& props);

        Value_t* buildValue() const;
        String_t getName() const;

     private:
        game::Session& m_session;
        const std::map<String_t, String_t>& m_properties;
    };

} }

#endif
