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

#include "fs/db/dbairportindex.h"

#include <QDebug>

namespace atools {
namespace fs {
namespace db {

void DbAirportIndex::add(const QString& airportIdent, int airportId)
{
  airportIndexMap[airportIdent] = airportId;
}

int DbAirportIndex::getAirportId(const QString& airportIdent, const QString& sourceObject)
{
  AirportIndexTypeConstIter it = airportIndexMap.find(airportIdent);
  if(it != airportIndexMap.end())
    return it.value();
  else
  {
    qWarning().nospace().noquote() << "Airport ID for ident " << airportIdent << " not found for " <<
    sourceObject;
    return -1;
  }
}

DbAirportIndex::~DbAirportIndex()
{
}

} // namespace writer
} // namespace fs
} // namespace atools
