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

#include "fs/bgl/ap/rw/runwayend.h"
#include "fs/bgl/converter.h"

namespace atools {
namespace fs {
namespace bgl {

QString RunwayEnd::patternToStr(rw::Pattern pattern)
{
  switch(pattern)
  {
    case rw::LEFT:
      return "L";

    case rw::RIGHT:
      return "R";
  }
  qWarning().nospace().noquote() << "Invalid runway pattern type " << pattern;
  return "UNKNOWN";
}

QDebug operator<<(QDebug out, const RunwayEnd& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << " RunwayEnd[Name " << record.getName()
  << ", left " << record.leftVasi
  << ", right " << record.rightVasi
  << ", " << record.approachLights
  << "]";
  return out;
}

RunwayEnd::RunwayEnd()
  : number(0), designator(0), offsetThreshold(0.0), blastPad(0.0), overrun(0.0), closedMarkings(false),
    stolMarkings(false), takeoff(false), landing(false), pattern(atools::fs::bgl::rw::LEFT)
{
}

RunwayEnd::~RunwayEnd()
{
}

QString RunwayEnd::getName() const
{
  return converter::runwayToStr(number, designator);
}

} // namespace bgl
} // namespace fs
} // namespace atools
