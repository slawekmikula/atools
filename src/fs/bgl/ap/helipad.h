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

#ifndef BGL_AIRPORTHELIPAD_H_
#define BGL_AIRPORTHELIPAD_H_

#include "fs/bgl/record.h"
#include "fs/bgl/ap/rw/runway.h"

#include <QString>

namespace atools {
namespace io {
class BinaryStream;
}
}

namespace atools {
namespace fs {
namespace bgl {

namespace helipad {
enum HelipadType
{
  NONE = 0,
  H = 1,
  SQUARE = 2,
  CIRCLE = 3,
  MEDICAL = 4
};

} // namespace helipad

class Helipad :
  public atools::fs::bgl::Record
{
public:
  Helipad();
  Helipad(const atools::fs::BglReaderOptions *options, atools::io::BinaryStream *bs);
  virtual ~Helipad();

  static QString helipadTypeToStr(atools::fs::bgl::helipad::HelipadType type);

  atools::fs::bgl::rw::Surface getSurface() const
  {
    return surface;
  }

  atools::fs::bgl::helipad::HelipadType getType() const
  {
    return type;
  }

  const atools::fs::bgl::BglPosition& getPosition() const
  {
    return position;
  }

  float getLength() const
  {
    return length;
  }

  float getWidth() const
  {
    return width;
  }

  float getHeading() const
  {
    return heading;
  }

  bool isClosed() const
  {
    return closed;
  }

  bool isTransparent() const
  {
    return transparent;
  }

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::Helipad& record);

  atools::fs::bgl::rw::Surface surface = atools::fs::bgl::rw::UNKNOWN;
  atools::fs::bgl::helipad::HelipadType type = atools::fs::bgl::helipad::NONE;
  atools::fs::bgl::BglPosition position;
  float length = 0.f, width = 0.f, heading = 0.f;
  bool transparent = false, closed = false;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif /* BGL_AIRPORTHELIPAD_H_ */
