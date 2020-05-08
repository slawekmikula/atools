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

#ifndef ATOOLS_BGL_NAV_TACAN_H
#define ATOOLS_BGL_NAV_TACAN_H

#include "fs/bgl/nav/navbase.h"
#include "fs/bgl/nav/ilsvor.h"

namespace atools {
namespace fs {
namespace bgl {

class Dme;

/*
 * TACAN
 */
class Tacan :
  public atools::fs::bgl::NavBase
{
public:
  Tacan(const atools::fs::NavDatabaseOptions *options, atools::io::BinaryStream *bs);
  virtual ~Tacan();

  QString getChannel() const
  {
    return QString::number(channel) + QChar(channelId);
  }

  /*
   * @return get the DME record for this VOR if available - otherwise null
   */
  const atools::fs::bgl::Dme *getDme() const
  {
    return dme;
  }

  /*
   * @return true if only DME
   */
  bool isDmeOnly() const
  {
    return dmeOnly;
  }

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::Tacan& record);

  char channelId;
  int channel;
  bool dmeOnly;
  atools::fs::bgl::Dme *dme = nullptr;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_BGL_NAV_TACAN_H
