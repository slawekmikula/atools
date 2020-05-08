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

#include "fs/db/ap/rw/runwaywriter.h"
#include "fs/db//datawriter.h"
#include "fs/bgl/util.h"
#include "fs/db/ap/airportwriter.h"
#include "fs/db/ap/rw/runwayendwriter.h"
#include "fs/db/runwayindex.h"
#include "fs/navdatabaseoptions.h"
#include "geo/calculations.h"
#include "atools.h"

#include <QString>

namespace atools {
namespace fs {
namespace db {

using atools::fs::bgl::Runway;
using atools::geo::meterToFeet;

void RunwayWriter::writeObject(const Runway *type)
{
  int runwayId = getNextId();

  QString apIdent = getDataWriter().getAirportWriter()->getCurrentAirportIdent();

  // Write runway ends before runway because we need the end ids to keep the foreign keys valid
  RunwayEndWriter *runwayEndWriter = getDataWriter().getRunwayEndWriter();

  runwayEndWriter->writeOne(type->getPrimary());
  int primaryEndId = runwayEndWriter->getCurrentId();
  getRunwayIndex()->add(apIdent, type->getPrimary().getName(), primaryEndId);

  runwayEndWriter->writeOne(type->getSecondary());
  int secondaryEndId = runwayEndWriter->getCurrentId();
  getRunwayIndex()->add(apIdent, type->getSecondary().getName(), secondaryEndId);

  if(getOptions().isVerbose())
    qDebug() << "Writing Runway for airport " << apIdent;

  // Write runway
  bind(":runway_id", runwayId);
  bind(":airport_id", getDataWriter().getAirportWriter()->getCurrentId());
  bind(":primary_end_id", primaryEndId);
  bind(":secondary_end_id", secondaryEndId);
  bind(":surface", Runway::surfaceToStr(type->getSurface()));
  bind(":length", roundToInt(meterToFeet(type->getLength())));
  bind(":width", roundToInt(meterToFeet(type->getWidth())));
  bind(":heading", type->getHeading());
  bind(":pattern_altitude", roundToPrecision(meterToFeet(type->getPatternAltitude()), 1));
  bind(":marking_flags", type->getMarkingFlags());
  bind(":edge_light", bgl::util::enumToStr(Runway::lightToStr, type->getEdgeLight()));
  bind(":center_light",
       bgl::util::enumToStr(Runway::lightToStr, type->getCenterLight()));
  bind(":has_center_red", type->isCenterRed());
  bind(":altitude", roundToInt(meterToFeet(type->getPosition().getAltitude())));

  bind(":primary_lonx", type->getPrimaryPosition().getLonX());
  bind(":primary_laty", type->getPrimaryPosition().getLatY());
  bind(":secondary_lonx", type->getSecondaryPosition().getLonX());
  bind(":secondary_laty", type->getSecondaryPosition().getLatY());

  bind(":lonx", type->getPosition().getLonX());
  bind(":laty", type->getPosition().getLatY());

  executeStatement();
}

} // namespace writer
} // namespace fs
} // namespace atools
