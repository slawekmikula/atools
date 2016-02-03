/*****************************************************************************
* Copyright 2015-2016 Alexander Barthel albar965@mailbox.org
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

#include "fs/bgl/ap/approach.h"
#include "fs/bgl/converter.h"
#include "fs/bgl/recordtypes.h"
#include "io/binarystream.h"

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

QString Approach::approachTypeToStr(ap::ApproachType type)
{
  switch(type)
  {
    case ap::GPS:
      return "GPS";

    case ap::VOR:
      return "VOR";

    case ap::NDB:
      return "NDB";

    case ap::ILS:
      return "ILS";

    case ap::LOCALIZER:
      return "LOCALIZER";

    case ap::SDF:
      return "SDF";

    case ap::LDA:
      return "LDA";

    case ap::VORDME:
      return "VORDME";

    case ap::NDBDME:
      return "NDBDME";

    case ap::RNAV:
      return "RNAV";

    case ap::LOCALIZER_BACKCOURSE:
      return "LOCALIZER_BACKCOURSE";
  }
  qWarning().nospace().noquote() << "Unknown approach type " << type;
  return QString();
}

QString Approach::approachFixTypeToStr(ap::ApproachFixType type)
{
  switch(type)
  {
    case ap::FIX_VOR:
      return "FIX_VOR";

    case ap::FIX_NDB:
      return "FIX_NDB";

    case ap::FIX_TERMINAL_NDB:
      return "FIX_TERMINAL_NDB";

    case ap::FIX_WAYPOINT:
      return "FIX_WAYPOINT";

    case ap::FIX_TERMINAL_WAYPOINT:
      return "FIX_TERMINAL_WAYPOINT";

    case ap::FIX_RUNWAY:
      return "FIX_RUNWAY";
  }
  qWarning().nospace().noquote() << "Unknown approach fix type " << type;
  return QString();
}

Approach::Approach(const BglReaderOptions *options, BinaryStream *bs)
  : Record(options, bs)
{
  bs->skip(1); // suffix
  runwayNumber = bs->readUByte();

  int typeFlags = bs->readUByte();
  type = static_cast<ap::ApproachType>(typeFlags & 0xf);
  runwayDesignator = (typeFlags >> 4) & 0x7;
  gpsOverlay = (typeFlags & 0x80) == 0x80;

  numTransitions = bs->readUByte();
  numLegs = bs->readUByte();
  numMissedLegs = bs->readUByte();

  unsigned int fixFlags = bs->readUInt();
  fixType = static_cast<ap::ApproachFixType>(fixFlags & 0xf);
  fixIdent = converter::intToIcao((fixFlags >> 5) & 0xfffffff, true);

  unsigned int fixIdentFlags = bs->readUInt();
  fixRegion = converter::intToIcao(fixIdentFlags & 0x7ff, true);
  fixAirportIdent = converter::intToIcao((fixIdentFlags >> 11) & 0x1fffff, true);

  altitude = bs->readFloat();
  heading = bs->readFloat();
  missedAltitude = bs->readFloat();

  while(bs->tellg() < startOffset + size)
  {
    Record r(options, bs);
    rec::ApprRecordType t = r.getId<rec::ApprRecordType>();

    switch(t)
    {
      case rec::TRANSITION:
        r.seekToStart();
        transitions.push_back(Transition(options, bs));
        break;

        // TODO read approach and transition legs
      case rec::LEGS:
      case rec::MISSED_LEGS:
      case rec::TRANSITION_LEGS:
        break;

      default:
        qWarning().nospace().noquote() << "Unexpected record type in approach record 0x" << hex << t << dec
                                       << " for airport ident " << fixAirportIdent;
    }
    r.seekToEnd();
  }
}

QDebug operator<<(QDebug out, const Approach& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const Record&>(record)
  << " Approach[type "
  << Approach::approachTypeToStr(record.type)
  << ", rwy " << record.getRunwayName()
  << ", gps overlay " << record.gpsOverlay
  << ", fix type " << Approach::approachFixTypeToStr(record.fixType)
  << ", fix " << record.fixIdent
  << ", fix region " << record.fixRegion
  << ", ap icao " << record.fixAirportIdent
  << ", alt " << record.altitude
  << ", hdg " << record.heading << endl;
  out << record.transitions;
  out << "]";
  return out;
}

Approach::~Approach()
{
}

QString Approach::getRunwayName() const
{
  return converter::runwayToStr(runwayNumber, runwayDesignator);
}

} // namespace bgl
} // namespace fs
} // namespace atools
