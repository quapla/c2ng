/**
  *  \file game/map/objecttype.hpp
  *  \brief Base class game::map::ObjectType
  */
#ifndef C2NG_GAME_MAP_OBJECTTYPE_HPP
#define C2NG_GAME_MAP_OBJECTTYPE_HPP

#include "afl/base/deletable.hpp"
#include "afl/base/signal.hpp"
#include "game/types.hpp"
#include "game/map/point.hpp"
#include "game/playerset.hpp"

namespace game { namespace map {

    class Object;
    class Universe;
    class Configuration;

    /** Object type descriptor.
        A class derived from ObjectType defines a set of objects ("all ships", "played starbases"), and iteration through them.

        An object is identified by a non-zero index.
        An valid index can be turned into an object using getObjectByIndex(), which returns null for invalid objects.

        The base class provides methods getNextIndex() and getPreviousIndex() for iteration.
        Those are not constrained to return only valid indexes (=isValidIndex() returns true).
        Therefore, users will most likely use findNextIndexWrap() etc.,
        which only return valid object indexes, and can optionally filter for marked objects.

        If the underlying set changes (as opposed to: the underlying objects change),
        the implementor must raise sig_setChange. */
    class ObjectType : public afl::base::Deletable {
     public:
        /** Virtual destructor. */
        virtual ~ObjectType();

        /** Get object, given an index.
            \param index Index
            \return object if it exists, null otherwise */
        virtual Object* getObjectByIndex(Id_t index) = 0;

        /** Get universe, given an index.
            Note that the universe can be null even for existing objects.
            \param index */
        virtual Universe* getUniverseByIndex(Id_t index) = 0;

        /** Get next index.
            The returned index need not be valid as per getObjectByIndex(),
            but the implementation must guarantee that repeated calls to getNextIndex()
            ultimately end up at 0, so that loops actually terminate.
            There is no requirement that the indexes are reported in a particular order, though.
            \param index Starting index. Can be 0 to obtain the first index.
            \return 0 if no more objects, otherwise an index */
        virtual Id_t getNextIndex(Id_t index) const = 0;

        /** Get previous index.
            The returned index need not be valid as per getObjectByIndex(),
            but the implementation must guarantee that repeated calls to getNextIndex()
            ultimately end up at 0, so that loops actually terminate.
            There is no requirement that the indexes are reported in a particular order, though.
            \param index Starting index. Can be 0 to obtain the last index.
            \return 0 if no more objects. Otherwise an index */
        virtual Id_t getPreviousIndex(Id_t index) const = 0;

        /*
         *  Derived functions
         */

        /** Find next object after index.
            Repeatedly calls getNextIndex() until it finds an object that exists (non-null getObjectByIndex()).

            This function is the same as findNextIndexNoWrap(index, false).
            It is intended for iteration.
            \param index Index to start search at. */
        Id_t findNextIndex(Id_t index) const;

        /** Check emptiness.
            \return true this type is empty, i.e. has no objects
            \return false this type is not empty, i.e. has objects */
        bool isEmpty() const;

        /** Check unit type.
            \return true this type has precisely one object
            \return false this type has more or fewer than one objects */
        bool isUnit() const;

        /** Count objects.
            \return Number of objects */
        int countObjects() const;

        /** Count objects at position.
            \param pt Count objects at this location
            \param owners Owners to accept
            \return Number of objects */
        int countObjectsAt(const Point pt, PlayerSet_t owners);

        /** Find nearest object.
            \param pt origin point
            \param config map configuration (for wrap awareness)
            \return Index of nearest object, 0 if none */
        Id_t findNearestIndex(const Point pt, const Configuration& config);

        /** Get previous object before index, with wrap.
            If the last object of a kind is reached, search starts again at the beginning.
            Can filter marked objects.
            \param index index to start search at.
            \param marked true to return only marked objects
            \return found index; 0 if none */
        Id_t findPreviousIndexWrap(Id_t index, bool marked);

        /** Get next object after index, with wrap.
            If the last object of a kind is reached, search starts again at the beginning.
            Can filter marked objects.
            \param index Index to start search at.
            \param marked true to return only marked objects
            \return found index; 0 if none */
        Id_t findNextIndexWrap(Id_t index, bool marked);

        /** Get previous object before index.
            Can filter marked objects.
            The returned object is guaranteed to exist.
            \param index Index to start search at.
            \param marked true to return only marked objects
            \return found index; 0 if none */
        Id_t findPreviousIndexNoWrap(Id_t index, bool marked);

        /** Get next object after index.
            Can filter marked objects.
            The returned object is guaranteed to exist.
            \param index Index to start search at.
            \param marked true to return only marked objects
            \return found index; 0 if none */
        Id_t findNextIndexNoWrap(Id_t index, bool marked);

        /** Find first object at a given position.
            \param pt Position
            \return found index; 0 if none. */
        Id_t findFirstObjectAt(Point pt);
        Id_t findNextObjectAt(Point pt, int id);

        /** Find object, given an Id.
            \param id Id
            \return found index; 0 if none. */
        Id_t findIndexForId(Id_t id);

        /** Notify all object listeners.
            Calls Object::notifyListeners() on all objects that are modified (Object::isDirty()).
            \retval false No object was dirty, no listeners notified
            \retval true Some objects were dirty */
        bool notifyObjectListeners();
 
        /** Called when the underlying set changes, i.e. objects come and go or are replaced by different objects.
            Called after the change.
 
            For simple changes, the integer can be a hint for users, i.e. the new Id of a renamed object.
            If the emitter doesn't want to give a hint, it can pass 0. */
        afl::base::Signal<void(Id_t)> sig_setChange;
    };

} }

#endif
