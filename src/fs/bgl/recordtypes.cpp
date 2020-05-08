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

#include "fs/bgl/recordtypes.h"

#include <QDebug>

namespace atools {
namespace fs {
namespace bgl {
namespace rec {

QString recordTypeStr(rec::RecordType type)
{
  switch(type)
  {
    case rec::AIRPORT:
      return "AIRPORT";

    case rec::WAYPOINT:
      return "WAYPOINT";

    case rec::AIRPORTSUMMARY:
      return "AIRPORTSUMMARY";

    case rec::ILS_VOR:
      return "ILS_VOR";

    case rec::NDB:
      return "NDB";

    case rec::SCENERYOBJECT:
      return "SCENERYOBJECT";

    case rec::MARKER:
      return "MARKER";

    case rec::BOUNDARY:
      return "BOUNDARY";

    case rec::GEOPOL:
      return "GEOPOL";

    case rec::NAMELIST:
      return "NAMELIST";

    case rec::VOR_ILS_ICAO_INDEX:
      return "VOR_ILS_ICAO_INDEX";

    case rec::NDB_ICAO_INDEX:
      return "NDB_ICAO_INDEX";

    case rec::WAYPOINT_ICAO_INDEX:
      return "WAYPOINT_ICAO_INDEX";
  }
  qWarning().nospace().noquote() << "Invalid record type " << type;
  return "INVALID";
}

QString airportRecordTypeStr(rec::AirportRecordType type)
{
  switch(type)
  {
    case rec::NAME:
      return "NAME";

    case rec::TOWER_OBJ:
      return "TOWER_OBJ";

    case rec::RUNWAY:
      return "RUNWAY";

    case rec::RUNWAY_P3D_V4:
      return "RUNWAY_P3D_V4";

    case rec::AIRPORT_WAYPOINT:
      return "AIRPORT_WAYPOINT";

    case rec::HELIPAD:
      return "HELIPAD";

    case rec::START:
      return "START";

    case rec::COM:
      return "COM";

    case rec::DELETE_AIRPORT:
      return "DELETE_AIRPORT";

    case rec::APRON_FIRST:
      return "APRON_FIRST";

    case rec::APRON_FIRST_P3D_V5:
      return "APRON_FIRST_P3D_V5";

    case rec::APRON_SECOND:
      return "APRON_SECOND";

    case rec::APRON_SECOND_P3D_V4:
      return "APRON_SECOND_P3D_V4";

    case rec::APRON_SECOND_P3D_V5:
      return "APRON_SECOND_P3D_V5";

    case rec::APRON_EDGE_LIGHTS:
      return "APRON_EDGE_LIGHTS";

    case rec::TAXI_POINT:
      return "TAXI_POINT";

    case rec::TAXI_POINT_P3DV5:
      return "TAXI_POINT_P3DV5";

    case rec::TAXI_PARKING:
      return "TAXI_PARKING";

    case rec::TAXI_PARKING_P3D_V5:
      return "TAXI_PARKING_P3D_V5";

    case rec::TAXI_PARKING_FS9:
      return "TAXI_PARKING_FS9";

    case rec::TAXI_PATH:
      return "TAXI_PATH";

    case rec::TAXI_PATH_P3D_V4:
      return "TAXI_PATH_P3D_V4";

    case rec::TAXI_PATH_P3D_V5:
      return "TAXI_PATH_P3D_V5";

    case rec::TAXI_NAME:
      return "TAXI_NAME";

    case rec::JETWAY:
      return "JETWAY";

    case rec::APPROACH:
      return "APPROACH";

    case rec::FENCE_BLAST:
      return "FENCE_BLAST";

    case rec::FENCE_BOUNDARY:
      return "FENCE_BOUNDARY";

    case rec::UNKNOWN_REC:
      return "UNKNOWN_REC";
  }
  qWarning().nospace().noquote() << "Invalid airport record type " << type;
  return "INVALID";
}

bool airportRecordTypeValid(rec::AirportRecordType type)
{
  switch(type)
  {
    case rec::AIRPORT_WAYPOINT:
    case rec::APPROACH:
    case rec::APRON_EDGE_LIGHTS:
    case rec::APRON_FIRST:
    case rec::APRON_FIRST_P3D_V5:
    case rec::APRON_SECOND:
    case rec::APRON_SECOND_P3D_V4:
    case rec::APRON_SECOND_P3D_V5:
    case rec::COM:
    case rec::DELETE_AIRPORT:
    case rec::FENCE_BLAST:
    case rec::FENCE_BOUNDARY:
    case rec::HELIPAD:
    case rec::JETWAY:
    case rec::NAME:
    case rec::RUNWAY:
    case rec::RUNWAY_P3D_V4:
    case rec::START:
    case rec::TAXI_NAME:
    case rec::TAXI_PARKING:
    case rec::TAXI_PARKING_FS9:
    case rec::TAXI_PARKING_P3D_V5:
    case rec::TAXI_PATH:
    case rec::TAXI_PATH_P3D_V4:
    case rec::TAXI_PATH_P3D_V5:
    case rec::TAXI_POINT:
    case rec::TAXI_POINT_P3DV5:
    case rec::TOWER_OBJ:
    case rec::UNKNOWN_REC:
      return true;
  }
  return false;
}

QString runwayRecordTypeStr(rec::RunwayRecordType type)
{
  switch(type)
  {
    case rec::OFFSET_THRESHOLD_PRIM:
      return "OFFSET_THRESHOLD_PRIM";

    case rec::OFFSET_THRESHOLD_SEC:
      return "OFFSET_THRESHOLD_SEC";

    case rec::BLAST_PAD_PRIM:
      return "BLAST_PAD_PRIM";

    case rec::BLAST_PAD_SEC:
      return "BLAST_PAD_SEC";

    case rec::OVERRUN_PRIM:
      return "OVERRUN_PRIM";

    case rec::OVERRUN_SEC:
      return "OVERRUN_SEC";

    case rec::VASI_PRIM_LEFT:
      return "VASI_PRIM_LEFT";

    case rec::VASI_PRIM_RIGHT:
      return "VASI_PRIM_RIGHT";

    case rec::VASI_SEC_LEFT:
      return "VASI_SEC_LEFT";

    case rec::VASI_SEC_RIGHT:
      return "VASI_SEC_RIGHT";

    case rec::APP_LIGHTS_PRIM:
      return "APP_LIGHTS_PRIM";

    case rec::APP_LIGHTS_SEC:
      return "APP_LIGHTS_SEC";
  }
  qWarning().nospace().noquote() << "Invalid runway record type " << type;
  return "INVALID";
}

QString approachRecordTypeStr(rec::ApprRecordType type)
{
  switch(type)
  {
    case rec::LEGS:
      return "LEGS";

    case rec::MISSED_LEGS:
      return "MISSED_LEGS";

    case rec::TRANSITION:
      return "TRANS";

    case rec::TRANSITION_LEGS:
      return "TRANS_LEGS";
  }
  qWarning().nospace().noquote() << "Invalid approach record type " << type;
  return "INVALID";
}

QString ilsvorRecordTypeStr(rec::IlsVorRecordType type)
{
  switch(type)
  {
    case rec::LOCALIZER:
      return "LOCALIZER";

    case rec::GLIDESLOPE:
      return "GLIDESLOPE";

    case rec::DME:
      return "DME";

    case rec::ILS_VOR_NAME:
      return "ILS_VOR_NAME";
  }
  qWarning().nospace().noquote() << "Invalid ILS/VOR type " << type;
  return "INVALID";
}

QString ndbRecordTypeStr(rec::NdbRecordType type)
{
  switch(type)
  {
    case rec::NDB_NAME:
      return "NDB_NAME";
  }
  qWarning().nospace().noquote() << "Invalid NDB type " << type;
  return "INVALID";
}

QString sceneryObjRecordTypeStr(rec::SceneryObjRecordType type)
{
  switch(type)
  {
    case rec::SCENERYOBJECT_LIB_OBJECT:
      return "SCENERYOBJECT_LIB_OBJECT";

    case rec::SCENERYOBJECT_ATTACHED_OBJECT:
      return "SCENERYOBJECT_ATTACHED_OBJECT";

    case rec::SCENERYOBJECT_EFFECT:
      return "SCENERYOBJECT_EFFECT";

    case rec::SCENERYOBJECT_GEN_BUILDING:
      return "SCENERYOBJECT_GEN_BUILDING";

    case rec::SCENERYOBJECT_WINDSOCK:
      return "SCENERYOBJECT_WINDSOCK";

    case rec::SCENERYOBJECT_EXT_BRIDGE:
      return "SCENERYOBJECT_EXT_BRIDGE";

    case rec::SCENERYOBJECT_TRIGGER:
      return "SCENERYOBJECT_TRIGGER";
  }
  qWarning().nospace().noquote() << "Invalid scenery object record type " << type;

  return "INVALID";
}

QString boundaryRecordTypeStr(rec::BoundaryRecordType type)
{
  switch(type)
  {
    case atools::fs::bgl::rec::BOUNDARY_COM:
      return "BOUNDARY_COM";

    case atools::fs::bgl::rec::BOUNDARY_NAME:
      return "BOUNDARY_NAME";

    case rec::BOUNDARY_LINES:
      return "BOUNDARY_LINES";
  }
  qWarning().nospace().noquote() << "Invalid boundary record type " << type;
  return "INVALID";
}

} // namespace rec
} // namespace bgl
} // namespace fs
} // namespace atools
