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

#include "fs/db/ap/taxipathwriter.h"
#include "fs/db/datawriter.h"
#include "fs/bgl/util.h"
#include "fs/navdatabaseoptions.h"
#include "fs/db/ap/airportwriter.h"
#include "fs/bgl/ap/rw/runway.h"
#include "fs/bgl/ap/taxipoint.h"
#include "geo/calculations.h"
#include "atools.h"

namespace atools {
namespace fs {
namespace db {

using atools::fs::bgl::TaxiPath;
using atools::fs::bgl::TaxiPoint;
using atools::fs::bgl::Runway;
using atools::geo::meterToFeet;

void TaxiPathWriter::writeObject(const TaxiPath *type)
{
  if(getOptions().isVerbose())
    qDebug() << "Writing TaxiPath for airport "
             << getDataWriter().getAirportWriter()->getCurrentAirportIdent();

  bind(":taxi_path_id", getNextId());
  bind(":airport_id", getDataWriter().getAirportWriter()->getCurrentId());
  bind(":type", TaxiPath::pathTypeToString(type->getType()));
  bind(":surface", Runway::surfaceToStr(type->getSurface()));
  bind(":width", roundToInt(meterToFeet(type->getWidth())));
  bind(":name", type->getName());

  bindBool(":is_draw_surface", type->isDrawSurface());
  bindBool(":is_draw_detail", type->isDrawDetail());

  bindBool(":has_centerline", type->hasCenterline());
  bindBool(":has_centerline_light", type->hasCenterlineLight());
  bindBool(":has_left_edge_light", type->hasLeftEdgeLight());
  bindBool(":has_right_edge_light", type->hasRightEdgeLight());

  bind(":start_type", bgl::util::enumToStr(TaxiPoint::pointTypeToString, type->getStartPoint().getType()));
  bind(":start_dir", bgl::util::enumToStr(TaxiPoint::dirToString, type->getStartPoint().getOrientation()));
  bind(":start_lonx", type->getStartPoint().getPosition().getLonX());
  bind(":start_laty", type->getStartPoint().getPosition().getLatY());

  bind(":end_type", bgl::util::enumToStr(TaxiPoint::pointTypeToString, type->getEndPoint().getType()));
  bind(":end_dir", bgl::util::enumToStr(TaxiPoint::dirToString, type->getEndPoint().getOrientation()));
  bind(":end_lonx", type->getEndPoint().getPosition().getLonX());
  bind(":end_laty", type->getEndPoint().getPosition().getLatY());

  executeStatement();
}

} // namespace writer
} // namespace fs
} // namespace atools
