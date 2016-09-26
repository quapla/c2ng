/**
  *  \file game/interface/beamfunction.cpp
  */

#include "game/interface/beamfunction.hpp"
#include "game/interface/beamcontext.hpp"
#include "interpreter/arguments.hpp"

// /* @q Beam(id:Int):Obj (Function, Context)
//    Access beam weapon properties.
//    Use as
//    | ForEach Beam Do ...
//    or
//    | With Beam(n) Do ...

//    @diff This function was available for use in %With under the name %Beams() since PCC 1.0.6.
//    Do not use the name %Beams in new code, it is not supported by PCC2; use %Beam instead.

//    @see int:index:group:beamproperty|Beam Properties
//    @since PCC 1.0.18, PCC2 1.99.8 */

game::interface::BeamFunction::BeamFunction(Session& session)
    : m_session(session)
{ }

// IndexableValue:
afl::data::Value*
game::interface::BeamFunction::get(interpreter::Arguments& args)
{
    // ex int/if/specif.h:IFBeamGet
    int32_t id;
    args.checkArgumentCount(1);
    if (m_session.getShipList().get() != 0
        && m_session.getRoot().get() != 0
        && interpreter::checkIntegerArg(id, args.getNext(), 1, getDimension(1)-1))
    {
        return new BeamContext(id, m_session.getShipList(), m_session.getRoot());
    }
    return 0;
}

void
game::interface::BeamFunction::set(interpreter::Arguments& /*args*/, afl::data::Value* /*value*/)
{
    throw interpreter::Error::notAssignable();
}

// CallableValue:
int32_t
game::interface::BeamFunction::getDimension(int32_t which)
{
    // ex int/if/specif.h:IFBeamDim
    return (which == 0
            ? 1
            : (m_session.getShipList().get() != 0
               ? m_session.getShipList()->beams().size()+1
               : 0));
}

interpreter::Context*
game::interface::BeamFunction::makeFirstContext()
{
    // ex int/if/specif.h:IFBeamMake
    if (game::spec::ShipList* list = m_session.getShipList().get()) {
        if (m_session.getRoot().get() != 0) {
            if (list->beams().size() > 0) {
                return new BeamContext(1, list, m_session.getRoot());
            }
        }
    }
    return 0;
}

game::interface::BeamFunction*
game::interface::BeamFunction::clone() const
{
    return new BeamFunction(m_session);
}

// BaseValue:
String_t
game::interface::BeamFunction::toString(bool /*readable*/) const
{
    return "#<array>";
}

void
game::interface::BeamFunction::store(interpreter::TagNode& /*out*/, afl::io::DataSink& /*aux*/, afl::charset::Charset& /*cs*/, interpreter::SaveContext* /*ctx*/) const
{
    throw interpreter::Error::notSerializable();
}
