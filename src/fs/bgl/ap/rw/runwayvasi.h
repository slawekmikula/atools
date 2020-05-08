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

#ifndef ATOOLS_BGL_RUNWAYVASI_H
#define ATOOLS_BGL_RUNWAYVASI_H

#include "fs/bgl/record.h"

#include <QList>

namespace atools {
namespace io {
class BinaryStream;
}
}

namespace atools {
namespace fs {
namespace bgl {

namespace rw {

enum VasiType
{
  NONE = 0x00,
  VASI21 = 0x01,
  VASI31 = 0x02,
  VASI22 = 0x03,
  VASI32 = 0x04,
  VASI23 = 0x05,
  VASI33 = 0x06,
  PAPI2 = 0x07,
  PAPI4 = 0x08,
  TRICOLOR = 0x09,
  PVASI = 0x0a,
  TVASI = 0x0b,
  BALL = 0x0c,
  APAP_PANELS = 0x0d
};

} // namespace rw

/*
 * VASI is a subrecord of runway. There is one for each runway end and one for each side.
 */
class RunwayVasi :
  public atools::fs::bgl::Record
{
public:
  RunwayVasi();
  RunwayVasi(const atools::fs::NavDatabaseOptions *options, atools::io::BinaryStream *bs);
  RunwayVasi(const atools::fs::bgl::RunwayVasi& other)
    : atools::fs::bgl::Record(other)
  {
    this->operator=(other);

  }

  virtual ~RunwayVasi();

  RunwayVasi& operator=(const atools::fs::bgl::RunwayVasi& other)
  {
    type = other.type;
    pitch = other.pitch;
    return *this;
  }

  /*
   * @return VASI pitch in degree
   */
  float getPitch() const
  {
    return pitch;
  }

  atools::fs::bgl::rw::VasiType getType() const
  {
    return type;
  }

  static QString vasiTypeToStr(atools::fs::bgl::rw::VasiType type);

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::RunwayVasi& record);

  atools::fs::bgl::rw::VasiType type;
  float pitch;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_BGL_RUNWAYVASI_H
