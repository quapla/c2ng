/**
  *  \file game/map/minefieldtype.hpp
  */
#ifndef C2NG_GAME_MAP_MINEFIELDTYPE_HPP
#define C2NG_GAME_MAP_MINEFIELDTYPE_HPP

#include "game/map/objectvector.hpp"
#include "game/map/minefield.hpp"
#include "game/map/objectvectortype.hpp"
#include "game/playerset.hpp"

namespace game { namespace map {

    class Universe;

    class MinefieldType : public ObjectVector<Minefield>,
                          public ObjectVectorType<Minefield>
    {
     public:
        MinefieldType(Universe& univ);
        ~MinefieldType();

        // ObjectVectorType:
        virtual bool isValid(const Minefield& obj) const;

        void erase(Id_t id);
        void setAllMinefieldsKnown(int player);
        void internalCheck(int currentTurn, const game::HostVersion& host, const game::config::HostConfiguration& config);

        // Missing from PCC2:
        // - begin/end --> use ObjectVector/ObjectVectorType methods
        // - addMinefieldReport --> use create()->addMinefieldReport() (use erase() for 0-unit report)

     private:
        // Players for which we ought to know all minefields
        PlayerSet_t m_allMinefieldsKnown;
    };

} }

#endif
