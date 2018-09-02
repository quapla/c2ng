/**
  *  \file game/map/configuration.hpp
  *  \brief Class game::map::Configuration
  */
#ifndef C2NG_GAME_MAP_CONFIGURATION_HPP
#define C2NG_GAME_MAP_CONFIGURATION_HPP

#include "game/map/point.hpp"
#include "game/config/hostconfiguration.hpp"
#include "game/hostversion.hpp"
#include "afl/string/string.hpp"
#include "game/config/userconfiguration.hpp"

namespace game { namespace map {

    class Configuration {
     public:
        enum Mode {
            Flat,
            Wrapped,
            Circular
        };

        /** Default constructor.
            Constructs an empty starchart configuration object.

            Change to PCC2: in PCC2, this function would have updated the user preferences.
            Use saveToConfiguration() to do that in c2ng. */
        Configuration();

        // Configuration inquiry
        Mode getMode() const;
        Point getCenter() const;
        Point getSize() const;
        Point getMinimumCoordinates() const;
        Point getMaximumCoordinates() const;
        int getCircularPrecision() const;
        int getCircularExcess() const;

        /*
         *  Configuration
         */

        /** Initialize from configuration.
            \param host Host version
            \param config Host configuration
            \param pref User configuration

            Change to PCC2: in PCC2, this function would have updated the user preferences.
            Use saveToConfiguration() to do that in c2ng. */
        void initFromConfiguration(const HostVersion& host,
                                   const game::config::HostConfiguration& config,
                                   const game::config::UserConfiguration& pref);

        /** Save to configuration.
            This updates the specified user configuration (preferences) object.
            \param pref User configuration object to update */
        void saveToConfiguration(game::config::UserConfiguration& pref);

        /** Set configuration.
            This overrides a previous configuration and marks it "not from host configuration".
            \param mode Wrap mode
            \param center Map center
            \param size Map size

            Change to PCC2: in PCC2, this function would have updated the user preferences.
            Use saveToConfiguration() to do that in c2ng. */
        void setConfiguration(Mode mode, Point center, Point size);

        /** Check for host configuration.
            \retval true the current map configuration is derived from the host configuration
            \retval false the current map configuration was set by the user */
        bool isSetFromHostConfiguration() const;

        /*
         *  Coordinate management
         */

        /** Check for point on map.

            Usage: This function is mostly used internally.

            \param pt point to check
            \retval true this point is on the map and accessible to players
            \retval false this point is not accessible to players */
        bool isOnMap(Point pt) const;

        /** Check for valid planet coordinates.
            Points may be on the map using isOnMap's rules, but by convention be treated as out-of-bounds.
            This is used by the ExploreMap add-on.

            Usage: to filter incoming planet coordinates

            \param pt point to check
            \return validity */
        bool isValidPlanetCoordinate(Point pt) const;

        /** Get canonical location.
            If any kind of wrap is active, this performs the "wrap" step normally performed by the host.

            Usage: any kind of "forward" prediction.
            For example, given a ship's after-movement coordinate (which could be outside the map),
            returns a new location on the map.

            \param pt point to check
            \return updated location */
        Point getCanonicalLocation(Point pt) const;

        /** Get canonical location of a point, simple version.
            This handles just rectangular wrap, where all instances of a location are equivalent.

            Usage: FIXME

            \param pt point to check
            \return updated location */
        Point getSimpleCanonicalLocation(Point pt) const;

        /** Get nearest alias of a point, simple version.
            This handles just rectangular wrap, where all instances of a location are equivalent.
            Returns the instance of \c pt that is closest to \c a (which might be outside the map).

            Usage: if \c pt is a ship's waypoint (e.g. a planet), and \c a is the ship's location,
            this function returns the desired waypoint.
            The waypoint will move the ship outside the map, but the host will move it in again.

            \param pt point to check
            \param a origin
            \return updated location */
        Point getSimpleNearestAlias(Point pt, Point a) const;

        /** Get number of map images that can map rectangles.
            \see getSimplePointAlias
            \return number of images */
        int  getNumRectangularImages() const;

        /** Get number of map images that can map points.
            \see getPointAlias
            \return number of images */
        int  getNumPointImages() const;

        /** Compute outside location for a point inside the map.
            This is an inverse operation to getCanonicalLocation.
            \param pt    [in] Point
            \param out   [out] Result will be produced here
            \param image [in] Index of map image to produce, [0,getNumPointImages()). 0=regular image.
            \param exact [in] true to request a perfect mapping, false to accept an inexact mapping
            \retval true this point could be mapped to the requested image
            \retval false this point could not be mapped */
        bool getPointAlias(Point pt, Point& out, int image, bool exact) const;

        /** Compute outside location for a point inside the map, simple version.
            This is well-suited to map known map objects in a fail-safe way.
            It does NOT map circular points to the outside.
            This is an inverse operation to getCanonicalLocation.

            \param pt    [in] Point
            \param image [in] Index of map image to produce, [0,getNumRectangularImages()). 0=regular image. */
        Point getSimplePointAlias(Point pt, int image) const;

        /** Get minimum distance between two points, considering map configuration.
            \param pt a,b points
            \return squared distance */
        int32_t getSquaredDistance(Point a, Point b) const;

        /*
         *  Sector numbers
         */

        /** Parse a sector number.
            \param s [in] user input
            \param result [out] result
            \retval true success; result has been updated
            \retval false failure; result was not modified */
        bool parseSectorNumber(const String_t& s, Point& result);

        /** Parse a sector number.
            \param n [in] user input
            \param result [out] result
            \retval true success; result has been updated
            \retval false failure; result was not modified */
        bool parseSectorNumber(int n, Point& result);

        /** Get sector number.
            The sector number is shown by PCC2 and PCC1.x, and agrees to Trevor Fuson's VGAMAP for a standard-sized map.
            \return sector number (100..499), zero if point is not in any numbered sector */
        int getSectorNumber(Point pt) const;

     private:
        void computeDerivedInformation();

        Mode m_mode;

        Point m_center;
        Point m_size;
        Point m_min;
        Point m_max;
        bool m_fromHostConfiguration;

        int m_circularPrecision;
        int m_circularExcess;
    };

} }

#endif
