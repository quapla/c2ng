/**
  *  \file game/interface/shipcontext.cpp
  *
  *  PCC2 comment:
  *
  *  Ship context.
  *  This class must override getObject() to achieve the same semantics as PCC 1.x
  *  (which seems to be the most useful semantics for the SHIP() array):
  *  - iteration uses ty_any_ships to iterate over everything visible
  *  - indexing uses ty_history_ships to allow accessing a history ship directly
  */

#include "game/interface/shipcontext.hpp"
#include "interpreter/nametable.hpp"
#include "game/interface/shipproperty.hpp"
#include "interpreter/typehint.hpp"
#include "afl/base/countof.hpp"
#include "interpreter/error.hpp"
#include "game/map/anyshiptype.hpp"
#include "game/turn.hpp"
#include "interpreter/propertyacceptor.hpp"
#include "afl/string/format.hpp"
#include "game/interface/objectcommand.hpp"
#include "game/interface/playerproperty.hpp"
#include "game/interface/componentproperty.hpp"

namespace game { namespace interface {
    // FIXME: move to separate file
    enum ShipMethod {
        ismMark,                    // 0
        ismUnmark,                  // 1
        ismSetComment,              // 2
        ismSetFCode,                // 3
        ismSetEnemy,                // 4
        ismSetSpeed,                // 5
        ismSetName,                 // 6
        ismSetMission,              // 7
        ismFixShip,                 // 8
        ismRecycleShip,             // 9
        ismSetWaypoint,             // 10
        ismCargoTransfer,           // 11
        ismCargoUnload,             // 12
        ismCargoUpload,             // 13
        ismSetFleet                 // 14
    };
} }

namespace {
    enum ShipDomain {
        ShipPropertyDomain,
        HullPropertyDomain,
        ComponentPropertyDomain,
        OwnerPropertyDomain,
        EnemyPropertyDomain,
        ShipMethodDomain
    };

    static const interpreter::NameTable ship_mapping[] = {
        { "AUX",                       game::interface::ispAuxName,                   ShipPropertyDomain,      interpreter::thString },
        { "AUX$",                      game::interface::ispAuxId,                     ShipPropertyDomain,      interpreter::thInt },
        { "AUX.AMMO",                  game::interface::ispAuxAmmo,                   ShipPropertyDomain,      interpreter::thInt },
        { "AUX.COUNT",                 game::interface::ispAuxCount,                  ShipPropertyDomain,      interpreter::thInt },
        { "AUX.SHORT",                 game::interface::ispAuxShort,                  ShipPropertyDomain,      interpreter::thString },
        { "BEAM",                      game::interface::ispBeamName,                  ShipPropertyDomain,      interpreter::thString },
        { "BEAM$",                     game::interface::ispBeamId,                    ShipPropertyDomain,      interpreter::thInt },
        { "BEAM.COUNT",                game::interface::ispBeamCount,                 ShipPropertyDomain,      interpreter::thInt },
        // { "BEAM.MAX",                  game::interface::ihpMaxBeams,                  HullPropertyDomain,      interpreter::thInt },
        { "BEAM.SHORT",                game::interface::ispBeamShort,                 ShipPropertyDomain,      interpreter::thString },
        { "CARGO.COLONISTS",           game::interface::ispCargoColonists,            ShipPropertyDomain,      interpreter::thInt },
        { "CARGO.D",                   game::interface::ispCargoD,                    ShipPropertyDomain,      interpreter::thInt },
        { "CARGO.FREE",                game::interface::ispCargoFree,                 ShipPropertyDomain,      interpreter::thInt },
        { "CARGO.M",                   game::interface::ispCargoM,                    ShipPropertyDomain,      interpreter::thInt },
        // { "CARGO.MAX",                 game::interface::ihpMaxCargo,                  HullPropertyDomain,      interpreter::thInt },
        // { "CARGO.MAXFUEL",             game::interface::ihpMaxFuel,                   HullPropertyDomain,      interpreter::thInt },
        { "CARGO.MONEY",               game::interface::ispCargoMoney,                ShipPropertyDomain,      interpreter::thInt },
        { "CARGO.N",                   game::interface::ispCargoN,                    ShipPropertyDomain,      interpreter::thInt },
        { "CARGO.STR",                 game::interface::ispCargoStr,                  ShipPropertyDomain,      interpreter::thString },
        { "CARGO.SUPPLIES",            game::interface::ispCargoSupplies,             ShipPropertyDomain,      interpreter::thInt },
        { "CARGO.T",                   game::interface::ispCargoT,                    ShipPropertyDomain,      interpreter::thInt },
        // { "CARGOTRANSFER",             game::interface::ismCargoTransfer,             ShipMethodDomain,        interpreter::thProcedure},
        // { "CARGOUNLOAD",               game::interface::ismCargoUnload,               ShipMethodDomain,        interpreter::thProcedure },
        // { "CARGOUPLOAD",               game::interface::ismCargoUpload,               ShipMethodDomain,        interpreter::thProcedure },
        { "CREW",                      game::interface::ispCrew,                      ShipPropertyDomain,      interpreter::thInt },
        // { "CREW.NORMAL",               game::interface::ihpMaxCrew,                   HullPropertyDomain,      interpreter::thInt },
        { "DAMAGE",                    game::interface::ispDamage,                    ShipPropertyDomain,      interpreter::thInt },
        { "ENEMY",                     game::interface::iplShortName,                 EnemyPropertyDomain,     interpreter::thString },
        { "ENEMY$",                    game::interface::ispEnemyId,                   ShipPropertyDomain,      interpreter::thInt },
        { "ENEMY.ADJ",                 game::interface::iplAdjName,                   EnemyPropertyDomain,     interpreter::thString },
        { "ENGINE",                    game::interface::ispEngineName,                ShipPropertyDomain,      interpreter::thString },
        { "ENGINE$",                   game::interface::ispEngineId,                  ShipPropertyDomain,      interpreter::thInt },
        // { "ENGINE.COUNT",              game::interface::ihpNumEngines,                HullPropertyDomain,      interpreter::thInt },
        { "FCODE",                     game::interface::ispFCode,                     ShipPropertyDomain,      interpreter::thString },
        { "FIGHTER.BAYS",              game::interface::ispFighterBays,               ShipPropertyDomain,      interpreter::thInt },
        { "FIGHTER.COUNT",             game::interface::ispFighterCount,              ShipPropertyDomain,      interpreter::thInt },
        // { "FIXSHIP",                   game::interface::ismFixShip,                   ShipMethodDomain,        interpreter::thProcedure },
        { "FLEET",                     game::interface::ispFleet,                     ShipPropertyDomain,      interpreter::thString },
        { "FLEET$",                    game::interface::ispFleetId,                   ShipPropertyDomain,      interpreter::thInt },
        { "FLEET.NAME",                game::interface::ispFleetName,                 ShipPropertyDomain,      interpreter::thString },
        { "FLEET.STATUS",              game::interface::ispFleetStatus,               ShipPropertyDomain,      interpreter::thString },
        { "HASFUNCTION",               game::interface::ispHasFunction,               ShipPropertyDomain,      interpreter::thArray },
        { "HEADING",                   game::interface::ispHeadingName,               ShipPropertyDomain,      interpreter::thString },
        { "HEADING$",                  game::interface::ispHeadingAngle,              ShipPropertyDomain,      interpreter::thInt },
        { "HULL",                      game::interface::icpName,                      ComponentPropertyDomain, interpreter::thString },
        { "HULL$",                     game::interface::icpId,                        ComponentPropertyDomain, interpreter::thInt },
        { "HULL.SHORT",                game::interface::icpNameShort,                 ComponentPropertyDomain, interpreter::thString },
        { "HULL.SPECIAL",              game::interface::ispHullSpecial,               ShipPropertyDomain,      interpreter::thString },
        { "ID",                        game::interface::ispId,                        ShipPropertyDomain,      interpreter::thInt },
        { "LEVEL",                     game::interface::ispLevel,                     ShipPropertyDomain,      interpreter::thInt },
        { "LOC",                       game::interface::ispLoc,                       ShipPropertyDomain,      interpreter::thString },
        { "LOC.X",                     game::interface::ispLocX,                      ShipPropertyDomain,      interpreter::thInt },
        { "LOC.Y",                     game::interface::ispLocY,                      ShipPropertyDomain,      interpreter::thInt },
        { "MARK",                      game::interface::ismMark,                      ShipMethodDomain,        interpreter::thProcedure },
        { "MARKED",                    game::interface::ispMarked,                    ShipPropertyDomain,      interpreter::thBool },
        { "MASS",                      game::interface::ispMass,                      ShipPropertyDomain,      interpreter::thInt },
        { "MISSION",                   game::interface::ispMissionName,               ShipPropertyDomain,      interpreter::thString },
        { "MISSION$",                  game::interface::ispMissionId,                 ShipPropertyDomain,      interpreter::thInt },
        { "MISSION.INTERCEPT",         game::interface::ispMissionIntercept,          ShipPropertyDomain,      interpreter::thInt },
        { "MISSION.SHORT",             game::interface::ispMissionShort,              ShipPropertyDomain,      interpreter::thString },
        { "MISSION.TOW",               game::interface::ispMissionTow,                ShipPropertyDomain,      interpreter::thInt },
        { "MOVE.ETA",                  game::interface::ispMoveETA,                   ShipPropertyDomain,      interpreter::thInt },
        { "MOVE.FUEL",                 game::interface::ispMoveFuel,                  ShipPropertyDomain,      interpreter::thInt },
        { "NAME",                      game::interface::ispName,                      ShipPropertyDomain,      interpreter::thString },
        { "ORBIT",                     game::interface::ispOrbitName,                 ShipPropertyDomain,      interpreter::thString },
        { "ORBIT$",                    game::interface::ispOrbitId,                   ShipPropertyDomain,      interpreter::thInt },
        { "OWNER",                     game::interface::iplShortName,                 OwnerPropertyDomain,     interpreter::thString },
        { "OWNER$",                    game::interface::iplId,                        OwnerPropertyDomain,     interpreter::thInt },
        { "OWNER.ADJ",                 game::interface::iplAdjName,                   OwnerPropertyDomain,     interpreter::thString },
        { "OWNER.REAL",                game::interface::ispRealOwner,                 ShipPropertyDomain,      interpreter::thInt },
        { "PLAYED",                    game::interface::ispPlayed,                    ShipPropertyDomain,      interpreter::thBool },
        // { "RECYCLESHIP",               game::interface::ismRecycleShip,               ShipMethodDomain,        interpreter::thProcedure },
        { "SCORE",                     game::interface::ispScore,                     ShipPropertyDomain,      interpreter::thArray },
        // { "SETCOMMENT",                game::interface::ismSetComment,                ShipMethodDomain,        interpreter::thProcedure },
        // { "SETENEMY",                  game::interface::ismSetEnemy,                  ShipMethodDomain,        interpreter::thProcedure },
        // { "SETFCODE",                  game::interface::ismSetFCode,                  ShipMethodDomain,        interpreter::thProcedure },
        // { "SETFLEET",                  game::interface::ismSetFleet,                  ShipMethodDomain,        interpreter::thProcedure },
        // { "SETMISSION",                game::interface::ismSetMission,                ShipMethodDomain,        interpreter::thProcedure },
        // { "SETNAME",                   game::interface::ismSetName,                   ShipMethodDomain,        interpreter::thProcedure },
        // { "SETSPEED",                  game::interface::ismSetSpeed,                  ShipMethodDomain,        interpreter::thProcedure },
        // { "SETWAYPOINT",               game::interface::ismSetWaypoint,               ShipMethodDomain,        interpreter::thProcedure },
        { "SPEED",                     game::interface::ispSpeedName,                 ShipPropertyDomain,      interpreter::thString },
        { "SPEED$",                    game::interface::ispSpeedId,                   ShipPropertyDomain,      interpreter::thInt },
        { "TASK",                      game::interface::ispTask,                      ShipPropertyDomain,      interpreter::thBool },
        { "TECH.HULL",                 game::interface::icpTech,                      ComponentPropertyDomain, interpreter::thInt },
        { "TORP",                      game::interface::ispTorpName,                  ShipPropertyDomain,      interpreter::thString },
        { "TORP$",                     game::interface::ispTorpId,                    ShipPropertyDomain,      interpreter::thInt },
        { "TORP.COUNT",                game::interface::ispTorpCount,                 ShipPropertyDomain,      interpreter::thInt },
        { "TORP.LCOUNT",               game::interface::ispTorpLCount,                ShipPropertyDomain,      interpreter::thInt },
        // { "TORP.LMAX",                 game::interface::ihpMaxTorpLaunchers,          HullPropertyDomain,      interpreter::thInt },
        { "TORP.SHORT",                game::interface::ispTorpShort,                 ShipPropertyDomain,      interpreter::thString },
        { "TRANSFER.SHIP",             game::interface::ispTransferShip,              ShipPropertyDomain,      interpreter::thBool },
        { "TRANSFER.SHIP.COLONISTS",   game::interface::ispTransferShipColonists,     ShipPropertyDomain,      interpreter::thInt },
        { "TRANSFER.SHIP.D",           game::interface::ispTransferShipD,             ShipPropertyDomain,      interpreter::thInt },
        { "TRANSFER.SHIP.ID",          game::interface::ispTransferShipId,            ShipPropertyDomain,      interpreter::thInt },
        { "TRANSFER.SHIP.M",           game::interface::ispTransferShipM,             ShipPropertyDomain,      interpreter::thInt },
        { "TRANSFER.SHIP.N",           game::interface::ispTransferShipN,             ShipPropertyDomain,      interpreter::thInt },
        { "TRANSFER.SHIP.NAME",        game::interface::ispTransferShipName,          ShipPropertyDomain,      interpreter::thInt },
        { "TRANSFER.SHIP.SUPPLIES",    game::interface::ispTransferShipSupplies,      ShipPropertyDomain,      interpreter::thInt },
        { "TRANSFER.SHIP.T",           game::interface::ispTransferShipT,             ShipPropertyDomain,      interpreter::thInt },
        { "TRANSFER.UNLOAD",           game::interface::ispTransferUnload,            ShipPropertyDomain,      interpreter::thBool },
        { "TRANSFER.UNLOAD.COLONISTS", game::interface::ispTransferUnloadColonists,   ShipPropertyDomain,      interpreter::thInt },
        { "TRANSFER.UNLOAD.D",         game::interface::ispTransferUnloadD,           ShipPropertyDomain,      interpreter::thInt },
        { "TRANSFER.UNLOAD.ID",        game::interface::ispTransferUnloadId,          ShipPropertyDomain,      interpreter::thInt },
        { "TRANSFER.UNLOAD.M",         game::interface::ispTransferUnloadM,           ShipPropertyDomain,      interpreter::thInt },
        { "TRANSFER.UNLOAD.N",         game::interface::ispTransferUnloadN,           ShipPropertyDomain,      interpreter::thInt },
        { "TRANSFER.UNLOAD.NAME",      game::interface::ispTransferUnloadName,        ShipPropertyDomain,      interpreter::thInt },
        { "TRANSFER.UNLOAD.SUPPLIES",  game::interface::ispTransferUnloadSupplies,    ShipPropertyDomain,      interpreter::thInt },
        { "TRANSFER.UNLOAD.T",         game::interface::ispTransferUnloadT,           ShipPropertyDomain,      interpreter::thInt },
        { "TYPE",                      game::interface::ispTypeStr,                   ShipPropertyDomain,      interpreter::thString },
        { "TYPE.SHORT",                game::interface::ispTypeChar,                  ShipPropertyDomain,      interpreter::thString },
        { "UNMARK",                    game::interface::ismUnmark,                    ShipMethodDomain,        interpreter::thProcedure },
        { "WAYPOINT",                  game::interface::ispWaypointName,              ShipPropertyDomain,      interpreter::thString },
        { "WAYPOINT.DIST",             game::interface::ispWaypointDistance,          ShipPropertyDomain,      interpreter::thFloat },
        { "WAYPOINT.DX",               game::interface::ispWaypointDX,                ShipPropertyDomain,      interpreter::thInt },
        { "WAYPOINT.DY",               game::interface::ispWaypointDY,                ShipPropertyDomain,      interpreter::thInt },
        { "WAYPOINT.PLANET",           game::interface::ispWaypointPlanetId,          ShipPropertyDomain,      interpreter::thInt },
        { "WAYPOINT.X",                game::interface::ispWaypointX,                 ShipPropertyDomain,      interpreter::thInt },
        { "WAYPOINT.Y",                game::interface::ispWaypointY,                 ShipPropertyDomain,      interpreter::thInt },
    };

    const size_t NUM_SHIP_PROPERTIES = countof(ship_mapping);

    bool lookupShipProperty(const afl::data::NameQuery& q, interpreter::World& world, interpreter::Context::PropertyIndex_t& result)
    {
        // Check user-defined properties
        afl::data::NameMap::Index_t ix = world.shipPropertyNames().getIndexByName(q);
        if (ix != afl::data::NameMap::nil) {
            result = ix + NUM_SHIP_PROPERTIES;
            return true;
        }

        // Check predefined properties
        return interpreter::lookupName(q, ship_mapping, result);
    }

    const game::spec::Hull* getShipHull(const game::map::Ship& sh,
                                        const game::spec::ShipList& list)
    {
        int n;
        if (sh.getHull().get(n)) {
            return list.hulls().get(n);
        } else {
            return 0;
        }
    }

    const game::interface::ObjectCommand::Function_t SHIP_METHODS[] = {
        game::interface::IFObjMark,                  // 0
        game::interface::IFObjUnmark,                // 1
        // IFShipSetComment,           // 2
        // IFShipSetFCode,             // 3
        // IFShipSetEnemy,             // 4
        // IFShipSetSpeed,             // 5
        // IFShipSetName,              // 6
        // IFShipSetMission,           // 7
        // IFShipFixShip,              // 8
        // IFShipRecycleShip,          // 9
        // IFShipSetWaypoint,          // 10
        // IFShipCargoTransfer,        // 11
        // IFShipCargoUnload,          // 12
        // IFShipCargoUpload,          // 13
        // IFShipSetFleet,             // 14
    };

}

game::interface::ShipContext::ShipContext(int id,
                                          Session& session,
                                          afl::base::Ptr<Root> root,
                                          afl::base::Ptr<Game> game,
                                          afl::base::Ptr<game::spec::ShipList> shipList)
    : m_id(id),
      m_session(session),
      m_root(root),
      m_game(game),
      m_shipList(shipList)
{ }

game::interface::ShipContext::~ShipContext()
{ }

// Context:
bool
game::interface::ShipContext::lookup(const afl::data::NameQuery& name, PropertyIndex_t& result)
{
    // ex IntShipContext::lookup
    if (name.startsWith("SHIP.")) {
        return lookupShipProperty(afl::data::NameQuery(name, 5), m_session.world(), result);
    } else {
        return lookupShipProperty(name, m_session.world(), result);
    }
}

void
game::interface::ShipContext::set(PropertyIndex_t index, afl::data::Value* value)
{
    // ex IntShipContext::set
    if (game::map::Ship* sh = getObject()) {
        if (index < NUM_SHIP_PROPERTIES) {
            // Builtin property
            switch (ShipDomain(ship_mapping[index].domain)) {
             case ShipPropertyDomain:
                setShipProperty(*sh, ShipProperty(ship_mapping[index].index), value, m_root);
                break;
             case HullPropertyDomain:
             case ComponentPropertyDomain:
             case OwnerPropertyDomain:
             case EnemyPropertyDomain:
             case ShipMethodDomain:
                throw interpreter::Error::notAssignable();
            }
        } else {
            // User property
            if (afl::data::Segment* seg = m_session.world().shipProperties().create(m_id)) {
                seg->set(index - NUM_SHIP_PROPERTIES, value);
            }
            sh->markDirty();
        }
    } else {
        // Nonexistant ships will still have a Ship object, so this branch is only taken for out-of-range Ids
        throw interpreter::Error::notAssignable();
    }
}

afl::data::Value*
game::interface::ShipContext::get(PropertyIndex_t index)
{
    if (game::map::Ship* sh = getObject()) {
        if (index < NUM_SHIP_PROPERTIES) {
            // Builtin property
            Root* root = m_root.get();
            game::spec::ShipList* list = m_shipList.get();
            int n;
            if (root != 0 && list != 0) {
                switch (ShipDomain(ship_mapping[index].domain)) {
                 case ShipPropertyDomain:
                    return getShipProperty(*sh,
                                           ShipProperty(ship_mapping[index].index),
                                           m_session.translator(),
                                           m_session.interface(),
                                           m_root,
                                           m_shipList,
                                           m_game);
             // case HullPropertyDomain:
             //    if (sh->getHullId().isKnown())
             //        return getHullProperty(sh->getHull(), IntHullProperty(ship_mapping[index].index));
             //    else
             //        return 0;
                 case ComponentPropertyDomain:
                    if (const game::spec::Hull* h = getShipHull(*sh, *list)) {
                        return getComponentProperty(*h, ComponentProperty(ship_mapping[index].index), *list);
                    } else {
                        return 0;
                    }
                 case OwnerPropertyDomain:
                    if (m_game.get() != 0 && sh->getOwner(n)) {
                        return getPlayerProperty(n,
                                                 PlayerProperty(ship_mapping[index].index),
                                                 root->playerList(),
                                                 *m_game,
                                                 root->hostConfiguration());
                    } else {
                        return 0;
                    }
                 case EnemyPropertyDomain:
                    if (m_game.get() != 0 && sh->getPrimaryEnemy().get(n)) {
                        return getPlayerProperty(n,
                                                 PlayerProperty(ship_mapping[index].index),
                                                 root->playerList(),
                                                 *m_game,
                                                 root->hostConfiguration());
                    } else {
                        return 0;
                    }
                 case ShipMethodDomain:
                    return new ObjectCommand(m_session, *sh, SHIP_METHODS[ship_mapping[index].index]);
                 default:
                    return 0;
                }
            } else {
                return 0;
            }
        } else {
            // User property
            return afl::data::Value::cloneOf(m_session.world().shipProperties().get(m_id, index - NUM_SHIP_PROPERTIES));
        }

    } else {
        // Nonexistant ships will still have a Ship object, so this branch is only taken for out-of-range Ids
        return 0;
    }
}

bool
game::interface::ShipContext::next()
{
    if (Game* game = m_game.get()) {
        if (int id = game::map::AnyShipType(game->currentTurn().universe()).findNextIndex(m_id)) {
            m_id = id;
            return true;
        }
    }
    return false;
}

game::interface::ShipContext*
game::interface::ShipContext::clone() const
{
    // ex IntShipContext::clone
    return new ShipContext(*this);
}

game::map::Ship*
game::interface::ShipContext::getObject()
{
    // ex IntShipContext::getObject
    if (Game* game = m_game.get()) {
        return game->currentTurn().universe().ships().get(m_id);
    } else {
        return 0;
    }
}

void
game::interface::ShipContext::enumProperties(interpreter::PropertyAcceptor& acceptor)
{
    // ex IntShipContext::enumProperties
    acceptor.enumNames(m_session.world().shipPropertyNames());
    acceptor.enumTable(ship_mapping);
}

// BaseValue:
String_t
game::interface::ShipContext::toString(bool /*readable*/) const
{
    // ex IntShipContext::toString
    return afl::string::Format("Ship(%d)", m_id);
}

void
game::interface::ShipContext::store(interpreter::TagNode& out, afl::io::DataSink& /*aux*/, afl::charset::Charset& /*cs*/, interpreter::SaveContext* /*ctx*/) const
{
    // ex IntShipContext::store
    out.tag = out.Tag_Ship;
    out.value = m_id;
}
