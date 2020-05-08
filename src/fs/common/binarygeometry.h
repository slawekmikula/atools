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

#ifndef ATOOLS_BINARYGEOMETRY_H
#define ATOOLS_BINARYGEOMETRY_H

#include "geo/linestring.h"

class QByteArray;

namespace atools {
namespace fs {
namespace common {

/*
 * FSX/P3D geometry for common use in database and client code.
 *
 * Writes a simple lat/long (not altitude) list in single floating point precision into a byte array which can be used
 * to write and read it into and from a database BLOB.
 */
class BinaryGeometry
{
public:
  BinaryGeometry();

  /* Sets line string geometry and does nothing else */
  BinaryGeometry(const atools::geo::LineString& value);

  /* Reads from byte array and provides line string */
  BinaryGeometry(const QByteArray& bytes);

  void readFromByteArray(const QByteArray& bytes);
  QByteArray writeToByteArray();

  const atools::geo::LineString& getGeometry() const
  {
    return geometry;
  }

  void swapGeometry(atools::geo::LineString& other)
  {
    geometry.swap(other);
  }

  void setGeometry(const atools::geo::LineString& value)
  {
    geometry = value;
  }

private:
  atools::geo::LineString geometry;
};

} // namespace common
} // namespace fs
} // namespace atools

#endif // ATOOLS_BINARYGEOMETRY_H
