/*****************************************************************************
* Copyright 2015-2020 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_FS_SC_SIMCONNECTAIRPLANE_H
#define ATOOLS_FS_SC_SIMCONNECTAIRPLANE_H

#include "geo/pos.h"
#include "fs/sc/simconnectdatabase.h"

#include <QString>

class QIODevice;

namespace xpc {
class XpConnect;
}

namespace atools {
namespace fs {
namespace online {
class OnlinedataManager;
}

namespace sc {

class SimConnectHandler;
class FgConnectHandler;
class SimConnectHandlerPrivate;
class SimConnectData;

// quint8
enum Category
{
  AIRPLANE,
  HELICOPTER,
  BOAT,
  CARRIER,
  FRIGATE,
  GROUNDVEHICLE,
  CONTROLTOWER,
  SIMPLEOBJECT,
  VIEWER,
  UNKNOWN
};

// quint8
enum EngineType
{
  PISTON = 0,
  JET = 1,
  NO_ENGINE = 2,
  HELO_TURBINE = 3,
  UNSUPPORTED = 4,
  TURBOPROP = 5
};

/*
 * Base aircraft that is used to transfer across network links. For user and AI aircraft.
 */
class SimConnectAircraft :
  public SimConnectDataBase
{
public:
  SimConnectAircraft();
  SimConnectAircraft(const SimConnectAircraft& other);
  virtual ~SimConnectAircraft() override;

  virtual void read(QDataStream & in);
  virtual void write(QDataStream& out) const;

  // fs data ----------------------------------------------------

  /* Mooney, Boeing, Actually aircraft model. */
  const QString& getAirplaneType() const
  {
    return airplaneType;
  }

  const QString& getAirplaneAirline() const
  {
    return airplaneAirline;
  }

  const QString& getAirplaneFlightnumber() const
  {
    return airplaneFlightnumber;
  }

  /* Beech Baron 58 Paint 1 */
  const QString& getAirplaneTitle() const
  {
    return airplaneTitle;
  }

  /* Short ICAO code MD80, BE58, etc. Actually type designator. */
  const QString& getAirplaneModel() const
  {
    return airplaneModel;
  }

  /* N71FS */
  const QString& getAirplaneRegistration() const
  {
    return airplaneReg;
  }

  /* Includes actual altitude in feet */
  atools::geo::Pos& getPosition()
  {
    return position;
  }

  /* Includes actual altitude in feet */
  const atools::geo::Pos& getPosition() const
  {
    return position;
  }

  /* Leaves altitude intact */
  void setCoordinates(atools::geo::Pos value)
  {
    value.setAltitude(position.getAltitude());
    position = value;
  }

  float getHeadingDegTrue() const
  {
    return headingTrueDeg;
  }

  float getHeadingDegMag() const
  {
    return headingMagDeg;
  }

  float getGroundSpeedKts() const
  {
    return groundSpeedKts;
  }

  float getIndicatedSpeedKts() const
  {
    return indicatedSpeedKts;
  }

  float getVerticalSpeedFeetPerMin() const
  {
    return verticalSpeedFeetPerMin;
  }

  float getIndicatedAltitudeFt() const
  {
    return indicatedAltitudeFt;
  }

  float getTrueAirspeedKts() const
  {
    return trueAirspeedKts;
  }

  float getMachSpeed() const
  {
    return machSpeed;
  }

  AircraftFlags getFlags() const
  {
    return flags;
  }

  void setFlags(AircraftFlags value)
  {
    flags = value;
  }

  Category getCategory() const
  {
    return category;
  }

  bool isAnyBoat() const
  {
    return category == BOAT || category == CARRIER || category == FRIGATE;
  }

  EngineType getEngineType() const
  {
    return engineType;
  }

  int getNumberOfEngines() const
  {
    return numberOfEngines;
  }

  unsigned int getObjectId() const
  {
    return objectId;
  }

  int getId() const
  {
    return static_cast<int>(objectId);
  }

  int getModelRadius() const
  {
    return modelRadiusFt;
  }

  /* Use an estimate based on engine type in ft - used for map display and not for HTML info */
  int getModelRadiusCorrected() const;

  int getWingSpan() const
  {
    return wingSpanFt;
  }

  int getDeckHeight() const
  {
    return deckHeight;
  }

  const QString& getFromIdent() const
  {
    return fromIdent;
  }

  const QString& getToIdent() const
  {
    return toIdent;
  }

  bool isOnGround() const
  {
    return flags & ON_GROUND;
  }

  bool isUser() const
  {
    return flags & IS_USER;
  }

  bool inCloud() const
  {
    return flags & IN_CLOUD;
  }

  bool inRain() const
  {
    return flags & IN_RAIN;
  }

  bool inSnow() const
  {
    return flags & IN_SNOW;
  }

  bool isSimPaused() const
  {
    return flags & SIM_PAUSED;
  }

  bool isSimReplay() const
  {
    return flags & SIM_REPLAY;
  }

  bool isOnline() const
  {
    return flags & SIM_ONLINE;
  }

  /* This simulator aircraft is a shadow of an online network aircraft (by matching callsign to reg) */
  bool isOnlineShadow() const
  {
    return flags & SIM_ONLINE_SHADOW;
  }

  /* Create artificially by mouse movements */
  bool isDebug() const
  {
    return debug;
  }

  /* true if not empty default initialized object */
  bool isValid() const
  {
    return position.isValid();
  }

  /* true if not empty default initialized object and not still at pos 0/0 */
  bool isFullyValid() const
  {
    return isValid() && !(groundSpeedKts < 5.0f && position.almostEqual(atools::geo::Pos(0.f, 0.f), 1.f));
  }

  /* Compares only registration, type and others */
  bool isSameAircraft(const SimConnectAircraft& other) const;

  /* Update all names which allows to replace translation key in MSFS with local translated aircraft names */
  void updateAircraftNames(const QString& airplaneTypeParam, const QString& airplaneAirlineParam,
                           const QString& airplaneTitleParam, const QString& airplaneModelParam);

private:
  friend class atools::fs::sc::SimConnectHandler;
  friend class atools::fs::sc::SimConnectHandlerPrivate;
  friend class atools::fs::sc::FgConnectHandler;
  friend class atools::fs::sc::SimConnectData;
  friend class xpc::XpConnect;
  friend class atools::fs::online::OnlinedataManager;

  QString airplaneTitle, airplaneType, airplaneModel, airplaneReg,
          airplaneAirline, airplaneFlightnumber, fromIdent, toIdent;

  atools::geo::Pos position;
  float headingTrueDeg = 0.f, headingMagDeg = 0.f, groundSpeedKts = 0.f, indicatedAltitudeFt = 0.f,
        indicatedSpeedKts = 0.f, trueAirspeedKts = 0.f,
        machSpeed = 0.f, verticalSpeedFeetPerMin = 0.f;
  quint16 modelRadiusFt = 0, wingSpanFt = 0, deckHeight = 0;

  quint32 objectId = 0L;

  AircraftFlags flags = atools::fs::sc::NONE;

  Category category;
  EngineType engineType = atools::fs::sc::UNSUPPORTED;
  quint8 numberOfEngines = 0;
  bool debug = false;
};

} // namespace sc
} // namespace fs
} // namespace atools

Q_DECLARE_METATYPE(atools::fs::sc::SimConnectAircraft);

#endif // ATOOLS_FS_SIMCONNECTAIRPLANE_H
