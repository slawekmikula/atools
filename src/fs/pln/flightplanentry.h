/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel alex@littlenavmap.org
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

#ifndef ATOOLS_FLIGHTPLANENTRY_H
#define ATOOLS_FLIGHTPLANENTRY_H

#include <QString>

#include "geo/pos.h"

namespace atools {
namespace fs {
namespace pln {

namespace entry {
enum WaypointType
{
  UNKNOWN,
  AIRPORT,
  INTERSECTION,
  VOR,
  NDB,
  USER
};

enum Flag
{
  NONE = 0,
  PROCEDURE = 1 << 1, /* Flight plan entry is any procedure leg */
  ALTERNATE = 1 << 2 /* Flight plan entry leads to an alternate airport */
};

Q_DECLARE_FLAGS(Flags, Flag);
Q_DECLARE_OPERATORS_FOR_FLAGS(atools::fs::pln::entry::Flags);

}

/*
 * Waypoint or airport as part of the flight plan. Also covers departure and destination airports.
 */
class FlightplanEntry
{
public:
  FlightplanEntry();
  FlightplanEntry(const atools::fs::pln::FlightplanEntry& other);
  ~FlightplanEntry();

  FlightplanEntry& operator=(const atools::fs::pln::FlightplanEntry& other);

  /*
   * @return waypoint type as string like "VOR", "Waypoint" or "User"
   */
  const QString& getWaypointTypeAsString() const;

  /* FS9 one character. First of getWaypointTypeAsString() */
  QString getWaypointTypeAsStringShort() const;

  atools::fs::pln::entry::WaypointType getWaypointType() const;

  /* Can use FSX or FS9 types */
  void setWaypointType(const QString& value);

  void setWaypointType(const atools::fs::pln::entry::WaypointType& value)
  {
    waypointType = value;
  }

  /*
   * @return ICAO ident of this waypoint
   */
  const QString& getWaypointId() const
  {
    return waypointId;
  }

  void setWaypointId(const QString& value)
  {
    waypointId = value;
  }

  /*
   * @return airway name if plan is an low alt or high alt flight plan
   */
  const QString& getAirway() const
  {
    return airway;
  }

  void setAirway(const QString& value)
  {
    airway = value;
  }

  /*
   * @return two letter ICAO region code
   */
  const QString& getIcaoRegion() const
  {
    return icaoRegion;
  }

  void setIcaoRegion(const QString& value)
  {
    icaoRegion = value;
  }

  /*
   * @return ICAO ident of this waypoint
   */
  const QString& getIcaoIdent() const
  {
    return icaoIdent;
  }

  void setIcaoIdent(const QString& value)
  {
    icaoIdent = value;
  }

  /*
   * @return coordinates of this waypoint
   */
  const geo::Pos& getPosition() const
  {
    return position;
  }

  void setPosition(const atools::geo::Pos& value)
  {
    position = value;
  }

  /* sets lat and lon but not altitude */
  void setCoords(const atools::geo::Pos& value)
  {
    position.setLonX(value.getLonX());
    position.setLatY(value.getLatY());
  }

  /* sets altitude in ft but not lat and lon */
  void setAltitude(const atools::geo::Pos& value)
  {
    position.setAltitude(value.getAltitude());
  }

  /* Altitude [ft] which is part of the position is used by some export functions */
  void setAltitude(float value)
  {
    position.setAltitude(value);
  }

  /* Do not save entry into the file if it is a procedure or an alternate airport */
  bool isNoSave() const
  {
    return (flags& entry::PROCEDURE) || (flags & entry::ALTERNATE);
  }

  bool operator==(const atools::fs::pln::FlightplanEntry& other);

  bool operator!=(const atools::fs::pln::FlightplanEntry& other)
  {
    return !operator==(other);
  }

  /* Name is not saved with PLN file */
  QString getName() const
  {
    return name;
  }

  void setName(const QString& value)
  {
    name = value;
  }

  /* Magnetic variance is not saved with PLN file */
  float getMagvar() const
  {
    return magvar;
  }

  void setMagvar(float value)
  {
    magvar = value;
  }

  /* NDB or VOR frequency, kHz * 100 or MHz * 1000 - not saved */
  int getFrequency() const
  {
    return frequency;
  }

  void setFrequency(int value)
  {
    frequency = value;
  }

  const atools::fs::pln::entry::Flags& getFlags() const
  {
    return flags;
  }

  void setFlags(const atools::fs::pln::entry::Flags& value)
  {
    flags = value;
  }

  void setFlag(atools::fs::pln::entry::Flag value, bool on = true)
  {
    flags.setFlag(value, on);
  }

private:
  friend QDebug operator<<(QDebug out, const atools::fs::pln::FlightplanEntry& record);

  static const QString& waypointTypeToString(atools::fs::pln::entry::WaypointType type);
  static atools::fs::pln::entry::WaypointType stringToWaypointType(const QString& str);

  atools::fs::pln::entry::WaypointType waypointType = entry::UNKNOWN;
  QString waypointId, airway, icaoRegion, icaoIdent, name;
  atools::geo::Pos position;
  atools::fs::pln::entry::Flags flags = atools::fs::pln::entry::NONE;
  float magvar = 0.f;
  int frequency = 0;
};

} // namespace pln
} // namespace fs
} // namespace atools

#endif // ATOOLS_FLIGHTPLANENTRY_H
