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

#include "fs/bgl/ap/transition.h"
#include "fs/bgl/ap/approach.h"
#include "fs/bgl/recordtypes.h"
#include "fs/bgl/converter.h"
#include "io/binarystream.h"
#include "fs/navdatabaseoptions.h"

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

QString Transition::transitionTypeToStr(ap::TransitionType type)
{
  switch(type)
  {
    case ap::FULL:
      return "F";

    case ap::DME:
      return "D";
  }
  qWarning().nospace().noquote() << "Invalid transition type " << type;
  return "INVALID";
}

QString Transition::transitionFixTypeToStr(ap::tfix::TransitionFixType type)
{
  switch(type)
  {
    case ap::tfix::VOR:
      return "V";

    case ap::tfix::NDB:
      return "N";

    case ap::tfix::TERMINAL_NDB:
      return "TN";

    /* From P3D v5 upwards - these are wrong types for this field taken from the XSD.
     * They will be converted to WAYPOINT. */
    case ap::tfix::MANUAL_TERMINATION:
    case ap::tfix::COURSE_TO_ALT:
    case ap::tfix::COURSE_TO_DIST:
    case ap::tfix::HEADING_TO_ALT:
    case ap::tfix::UNKNOWN_TYPE:
    case ap::tfix::WAYPOINT:
      return "W";

    case ap::tfix::TERMINAL_WAYPOINT:
      return "TW";
  }
  qWarning().nospace().noquote() << "Invalid transition fix type " << type;
  return "INVALID";
}

Transition::Transition(const NavDatabaseOptions *options, BinaryStream *bs)
  : Record(options, bs)
{
  type = static_cast<ap::TransitionType>(bs->readUByte());

  int numLegs = bs->readUByte();
  Q_UNUSED(numLegs);

  unsigned int transFixFlags = bs->readUInt();
  transFixType = static_cast<ap::tfix::TransitionFixType>(transFixFlags & 0xf);
  transFixIdent = converter::intToIcao((transFixFlags >> 5) & 0xfffffff, true);

  unsigned int fixIdentFlags = bs->readUInt();
  fixRegion = converter::intToIcao(fixIdentFlags & 0x7ff, true);
  fixAirportIdent = converter::intToIcao((fixIdentFlags >> 11) & 0x1fffff, true);

  altitude = bs->readFloat();

  if(type == ap::DME)
  {
    dmeIdent = converter::intToIcao(bs->readUInt());
    unsigned int tempFixIdentFlags = bs->readUInt();
    dmeRegion = converter::intToIcao(tempFixIdentFlags & 0x7ff, true);
    dmeAirportIdent = converter::intToIcao((tempFixIdentFlags >> 11) & 0x1fffff, true);
    dmeRadial = bs->readInt();
    dmeDist = bs->readFloat();
  }
  else
  {
    dmeIdent.clear();
    dmeRegion.clear();
    dmeAirportIdent.clear();
    dmeRadial = 0;
    dmeDist = 0.f;
  }

  while(bs->tellg() < startOffset + size)
  {
    Record r(options, bs);
    rec::ApprRecordType t = r.getId<rec::ApprRecordType>();
    if(checkSubRecord(r))
      return;

    switch(t)
    {
      case rec::TRANSITION_LEGS:
      case rec::TRANSITION_LEGS_MSFS:
        if(options->isIncludedNavDbObject(type::APPROACHLEG))
        {
          int num = bs->readUShort();
          for(int i = 0; i < num; i++)
            legs.append(ApproachLeg(bs, false, t == rec::TRANSITION_LEGS_MSFS));
        }
        break;

      default:
        qWarning().nospace().noquote() << "Unexpected record type in transition record 0x" << hex << t << dec
                                       << " for airport ident " << fixAirportIdent;
    }
    r.seekToEnd();
  }

}

QDebug operator<<(QDebug out, const Transition& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const Record&>(record)
                          << " Transition[type " << Transition::transitionTypeToStr(record.type)
                          << ", transFixType " << Transition::transitionFixTypeToStr(record.transFixType)
                          << ", transFixIdent " << record.transFixIdent
                          << ", fixRegion " << record.fixRegion
                          << ", airportIdent " << record.fixAirportIdent
                          << ", altitude " << record.altitude
                          << ", dmeIdent " << record.dmeIdent
                          << ", dmeRegion " << record.dmeRegion
                          << ", dmeAirportIdent " << record.dmeAirportIdent
                          << ", dmeRadial " << record.dmeRadial
                          << ", dmeDist " << record.dmeDist << endl;
  out << record.legs;
  out << "]";
  return out;
}

Transition::~Transition()
{
}

} // namespace bgl
} // namespace fs
} // namespace atools
