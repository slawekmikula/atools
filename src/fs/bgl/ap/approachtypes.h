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

#ifndef ATOOLS_APPROACHTYPES_H
#define ATOOLS_APPROACHTYPES_H

#include <QString>

namespace atools {
namespace fs {
namespace bgl {

namespace ap {

enum ApproachType
{
  GPS = 0x01,
  VOR = 0x02,
  NDB = 0x03,
  ILS = 0x04,
  LOCALIZER = 0x05,
  SDF = 0x06,
  LDA = 0x07,
  VORDME = 0x08,
  NDBDME = 0x09,
  RNAV = 0x0a,
  LOCALIZER_BACKCOURSE = 0x0b,
  TACAN = VORDME | ILS /* From P3D v4 upwards */
};

QString approachTypeToStr(atools::fs::bgl::ap::ApproachType type);

namespace fix {

enum ApproachFixType
{
  NONE = 0,
  VOR = 2,
  NDB = 3,
  TERMINAL_NDB = 4,
  WAYPOINT = 5,
  TERMINAL_WAYPOINT = 6,
  LOCALIZER = 8,
  RUNWAY = 9,

  /* From P3D v5 upwards - these are wrong types for this field taken from the XSD.
   * They will be converted to WAYPOINT. */
  // <xs:enumeration value="NDB" />
  // <xs:enumeration value="COURSE_TO_DIST" />
  // <xs:enumeration value="COURSE_TO_ALT" />
  // <xs:enumeration value="HEADING_TO_ALT" />
  // <xs:enumeration value="MANUAL_TERMINATION" />
  // <xs:enumeration value="TERMINAL_NDB" />
  // <xs:enumeration value="TERMINAL_WAYPOINT" />
  // <xs:enumeration value="VOR" />
  // <xs:enumeration value="WAYPOINT" />
  // <xs:enumeration value="RUNWAY" />
  // <xs:enumeration value="LOCALIZER" />
  MANUAL_TERMINATION = 11,
  COURSE_TO_ALT = 12,
  COURSE_TO_DIST = 13,
  HEADING_TO_ALT = 14
};

}

QString approachFixTypeToStr(atools::fs::bgl::ap::fix::ApproachFixType type);

} // namespace ap
} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_APPROACHTYPES_H
