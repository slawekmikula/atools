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

#include "fs/xp/xpnavwriter.h"

#include "fs/common/airportindex.h"
#include "fs/util/tacanfrequencies.h"
#include "fs/progresshandler.h"
#include "fs/navdatabaseoptions.h"
#include "fs/common/magdecreader.h"

#include "geo/pos.h"
#include "geo/calculations.h"
#include "sql/sqlutil.h"
#include "sql/sqlquery.h"

using atools::sql::SqlQuery;
using atools::sql::SqlUtil;

namespace atools {
namespace fs {
namespace xp {

// 4  36.692211111    3.217516667      131    11030    25    232.655   AG DAAG DA 23 ILS-cat-III
// 6  36.710150000    3.249166667      131    11030    25 300232.655   AG DAAG DA 23 GS
// 12  36.710150000    3.249166667      131    11030    25      0.000   AG DAAG DA HOUARI BOUMEDIENE DME-ILS
enum FieldIndex
{
  ROWCODE = 0,
  LATY = 1,
  LONX = 2,
  ALT = 3,
  FREQ = 4,
  RANGE = 5,
  HDG = 6,
  MAGVAR = HDG,
  IDENT = 7,
  AIRPORT = 8,
  REGION = 9,
  RW = 10,
  NAME = 11
};

XpNavWriter::XpNavWriter(atools::sql::SqlDatabase& sqlDb, atools::fs::common::AirportIndex *airportIndexParam,
                         const NavDatabaseOptions& opts, ProgressHandler *progressHandler,
                         atools::fs::NavDatabaseErrors *navdatabaseErrors)
  : XpWriter(sqlDb, opts, progressHandler, navdatabaseErrors), airportIndex(airportIndexParam)
{
  initQueries();
}

atools::fs::xp::XpNavWriter::~XpNavWriter()
{
  deInitQueries();
}

void XpNavWriter::writeVor(const QStringList& line, int curFileId, bool dmeOnly)
{
  // 25, 40 and 130 correspond to VORs classified as terminal, low and high.﻿
  // 125 rang﻿e is where the VOR has n﻿o﻿ published ter﻿m/low/high classifica﻿ti﻿on.
  // These VORs might have the power output of a high VOR, but are not tested/certified to fulfill the
  // high altitude SV﻿V﻿﻿.

  int range = at(line, RANGE).toInt();
  QString type;
  QString rangeType;
  if(range < 30)
    rangeType = "T";
  else if(range < 50)
    rangeType = "L";
  else
    rangeType = "H";

  QString suffix = line.last().toUpper();
  if(suffix == "VOR" || suffix == "DME" || suffix == "VOR-DME" || suffix == "VOR/DME")
    type = rangeType;
  else if(suffix == "VORTAC")
    type = "VT" + rangeType;
  else if(suffix == "TACAN")
    type = "TC";

  bool hasDme = suffix == "DME" || suffix == "VORTAC" || suffix == "VOR-DME" || suffix == "VOR/DME";
  int frequency = at(line, FREQ).toInt();

  insertVorQuery->bindValue(":vor_id", ++curVorId);
  insertVorQuery->bindValue(":file_id", curFileId);
  insertVorQuery->bindValue(":ident", at(line, IDENT));
  insertVorQuery->bindValue(":name", line.mid(RW, line.size() - 11).join(" "));
  insertVorQuery->bindValue(":region", at(line, REGION));
  insertVorQuery->bindValue(":type", type);
  insertVorQuery->bindValue(":frequency", frequency * 10);
  insertVorQuery->bindValue(":range", range);
  insertVorQuery->bindValue(":mag_var", at(line, MAGVAR).toFloat());
  insertVorQuery->bindValue(":dme_only", dmeOnly);
  insertVorQuery->bindValue(":airport_id", airportIndex->getAirportId(at(line, AIRPORT)));

  if(suffix == "TACAN" || suffix == "VORTAC")
    insertVorQuery->bindValue(":channel", atools::fs::util::tacanChannelForFrequency(frequency));
  else
    insertVorQuery->bindValue(":channel", QVariant(QVariant::String));

  if(hasDme)
  {
    insertVorQuery->bindValue(":dme_altitude", at(line, ALT).toInt());
    insertVorQuery->bindValue(":dme_lonx", at(line, LONX).toFloat());
    insertVorQuery->bindValue(":dme_laty", at(line, LATY).toFloat());
    insertVorQuery->bindValue(":altitude", at(line, ALT).toInt());
  }
  else
  {
    insertVorQuery->bindValue(":dme_altitude", QVariant(QVariant::Int));
    insertVorQuery->bindValue(":dme_lonx", QVariant(QVariant::Double));
    insertVorQuery->bindValue(":dme_laty", QVariant(QVariant::Double));

    // VOR only - unlikely to have an elevation
    if(at(line, ALT).toInt() != 0)
      insertVorQuery->bindValue(":altitude", at(line, ALT).toInt());
    else
      insertVorQuery->bindValue(":altitude", QVariant(QVariant::Int));
  }

  insertVorQuery->bindValue(":lonx", at(line, LONX).toFloat());
  insertVorQuery->bindValue(":laty", at(line, LATY).toFloat());

  insertVorQuery->exec();

  progress->incNumVors();
}

void XpNavWriter::writeNdb(const QStringList& line, int curFileId, const XpWriterContext& context)
{
  int range = at(line, RANGE).toInt();

  QString type;
  if(range < 24)
    type = "CP";
  else if(range < 40)
    type = "MH";
  else if(range < 70)
    type = "H";
  else
    type = "HH";

  atools::geo::Pos pos(at(line, LONX).toFloat(), at(line, LATY).toFloat());

  insertNdbQuery->bindValue(":ndb_id", ++curNdbId);
  insertNdbQuery->bindValue(":file_id", curFileId);
  insertNdbQuery->bindValue(":ident", at(line, IDENT));
  insertNdbQuery->bindValue(":name", line.mid(RW, line.size() - 11).join(" "));
  insertNdbQuery->bindValue(":region", at(line, REGION));
  insertNdbQuery->bindValue(":type", type);
  insertNdbQuery->bindValue(":frequency", at(line, FREQ).toInt() * 100);
  insertNdbQuery->bindValue(":range", range);
  insertNdbQuery->bindValue(":airport_id", airportIndex->getAirportId(at(line, AIRPORT)));
  // NDBs never have an altitude
  if(at(line, ALT).toInt() != 0)
    insertNdbQuery->bindValue(":altitude", at(line, ALT).toInt());
  else
    insertNdbQuery->bindValue(":altitude", QVariant(QVariant::Int));
  insertNdbQuery->bindValue(":mag_var", context.magDecReader->getMagVar(pos));
  insertNdbQuery->bindValue(":lonx", pos.getLonX());
  insertNdbQuery->bindValue(":laty", pos.getLatY());

  insertNdbQuery->exec();

  progress->incNumNdbs();
}

void XpNavWriter::writeMarker(const QStringList& line, int curFileId, NavRowCode rowCode)
{
  QString type;
  if(rowCode == OM)
    type = "OUTER";
  else if(rowCode == MM)
    type = "MIDDLE";
  else if(rowCode == IM)
    type = "INNER";

  insertMarkerQuery->bindValue(":marker_id", ++curMarkerId);
  insertMarkerQuery->bindValue(":file_id", curFileId);
  insertMarkerQuery->bindValue(":region", at(line, REGION));
  insertMarkerQuery->bindValue(":type", type);
  insertMarkerQuery->bindValue(":ident", at(line, IDENT));
  insertMarkerQuery->bindValue(":heading", at(line, HDG).toFloat());
  insertMarkerQuery->bindValue(":altitude", at(line, ALT).toInt());
  insertMarkerQuery->bindValue(":lonx", at(line, LONX).toFloat());
  insertMarkerQuery->bindValue(":laty", at(line, LATY).toFloat());

  insertMarkerQuery->exec();

  progress->incNumMarker();
}

void XpNavWriter::writeIls(const QStringList& line, int curFileId, const XpWriterContext& context)
{
  Q_UNUSED(curFileId);

  const QString& airportIdent = at(line, AIRPORT);
  const QString& airportRegion = at(line, REGION);
  const QString& ilsIdent = at(line, IDENT);

  if(airportIndex->getAirportIlsId(airportIdent, airportRegion, ilsIdent) != -1)
  {
    // Remember what was skipped in this file so DME and GS update functions can skip too
    airportIndex->addSkippedAirportIls(airportIdent, airportRegion, ilsIdent);
    return;
  }

  airportIndex->addAirportIls(airportIdent, airportRegion, ilsIdent, ++curIlsId);

  const QString& runwayName = at(line, RW);
  atools::geo::Pos pos(at(line, LONX).toFloat(), at(line, LATY).toFloat());

  insertIlsQuery->bindValue(":ils_id", curIlsId);
  insertIlsQuery->bindValue(":frequency", at(line, FREQ).toInt() * 10);
  insertIlsQuery->bindValue(":range", at(line, RANGE).toInt());
  insertIlsQuery->bindValue(":loc_heading", at(line, HDG).toFloat());
  insertIlsQuery->bindValue(":ident", ilsIdent);
  insertIlsQuery->bindValue(":loc_airport_ident", airportIdent);
  insertIlsQuery->bindValue(":region", at(line, REGION));
  insertIlsQuery->bindValue(":loc_runway_name", runwayName);
  insertIlsQuery->bindValue(":name", line.mid(NAME).join(" ").toUpper());
  insertIlsQuery->bindValue(":loc_runway_end_id", airportIndex->getRunwayEndId(airportIdent, runwayName));
  insertIlsQuery->bindValue(":altitude", at(line, ALT).toInt());
  insertIlsQuery->bindValue(":lonx", pos.getLonX());
  insertIlsQuery->bindValue(":laty", pos.getLatY());

  insertIlsQuery->bindValue(":mag_var", context.magDecReader->getMagVar(pos));
  insertIlsQuery->bindValue(":loc_width", QVariant(QVariant::Int)); // Not available
  insertIlsQuery->bindValue(":has_backcourse", 0);

  int length = atools::geo::nmToMeter(FEATHER_LEN_NM);
  // Calculate the display of the ILS feather
  float ilsHeading = atools::geo::normalizeCourse(atools::geo::opposedCourseDeg(at(line, HDG).toFloat()));
  atools::geo::Pos p1 = pos.endpoint(length, ilsHeading - FEATHER_WIDTH / 2.f).normalize();
  atools::geo::Pos p2 = pos.endpoint(length, ilsHeading + FEATHER_WIDTH / 2.f).normalize();
  float featherWidth = p1.distanceMeterTo(p2);
  atools::geo::Pos pmid = pos.endpoint(length - featherWidth / 2, ilsHeading).normalize();

  insertIlsQuery->bindValue(":end1_lonx", p1.getLonX());
  insertIlsQuery->bindValue(":end1_laty", p1.getLatY());
  insertIlsQuery->bindValue(":end_mid_lonx", pmid.getLonX());
  insertIlsQuery->bindValue(":end_mid_laty", pmid.getLatY());
  insertIlsQuery->bindValue(":end2_lonx", p2.getLonX());
  insertIlsQuery->bindValue(":end2_laty", p2.getLatY());

  insertIlsQuery->exec();
  insertIlsQuery->clearBoundValues();
  progress->incNumIls();
}

void XpNavWriter::updateIlsGlideslope(const QStringList& line)
{
  const QString& airportIdent = at(line, AIRPORT);
  const QString& airportRegion = at(line, REGION);
  const QString& ilsIdent = at(line, IDENT);

  if(airportIndex->hasSkippedAirportIls(airportIdent, airportRegion, ilsIdent))
    // Already skipped in this file
    return;

  int ilsId = airportIndex->getAirportIlsId(airportIdent, airportRegion, ilsIdent);

  float pitchHdg = at(line, HDG).toFloat();
  float pitch = std::floor(pitchHdg / 1000.f) / 100.f;
  updateIlsGsQuery->bindValue(":id", ilsId);
  updateIlsGsQuery->bindValue(":gs_range", at(line, RANGE).toInt());
  updateIlsGsQuery->bindValue(":gs_pitch", pitch);
  updateIlsGsQuery->bindValue(":gs_altitude", at(line, ALT).toInt());
  updateIlsGsQuery->bindValue(":gs_lonx", at(line, LONX).toFloat());
  updateIlsGsQuery->bindValue(":gs_laty", at(line, LATY).toFloat());
  updateIlsGsQuery->exec();
  updateIlsGsQuery->clearBoundValues();
}

void XpNavWriter::updateIlsDme(const QStringList& line)
{
  const QString& airportIdent = at(line, AIRPORT);
  const QString& airportRegion = at(line, REGION);
  const QString& ilsIdent = at(line, IDENT);

  if(airportIndex->hasSkippedAirportIls(airportIdent, airportRegion, ilsIdent))
    // Already skipped in this file
    return;

  int ilsId = airportIndex->getAirportIlsId(airportIdent, airportRegion, ilsIdent);

  updateIlsDmeQuery->bindValue(":id", ilsId);
  updateIlsDmeQuery->bindValue(":dme_range", at(line, RANGE).toInt());
  updateIlsDmeQuery->bindValue(":dme_altitude", at(line, ALT).toInt());
  updateIlsDmeQuery->bindValue(":dme_lonx", at(line, LONX).toFloat());
  updateIlsDmeQuery->bindValue(":dme_laty", at(line, LATY).toFloat());
  updateIlsDmeQuery->exec();
  updateIlsDmeQuery->clearBoundValues();
}

void XpNavWriter::write(const QStringList& line, const XpWriterContext& context)
{
  ctx = &context;

  // lat lon
  // ("28.000708333", "-83.423330556", "KNOST", "ENRT", "K7")
  // if(at(line, IDENT) == "OEV")
  // qDebug() << "OEV";

  NavRowCode rowCode = static_cast<NavRowCode>(at(line, ROWCODE).toInt());

  // Glideslope records must come later in the file than their associated localizer
  // LTP/FTP records must come later in the file than their associated FPAP
  // Paired DME records must come later in the file than their associated VOR or TACAN
  switch(rowCode)
  {
    // 2 NDB (Non-Directional Beacon) Includes NDB component of Locator Outer Markers (LOM)
    case NDB:
      if(options.isIncludedNavDbObject(atools::fs::type::NDB))
        writeNdb(line, context.curFileId, context);
      break;

    // 3 VOR (including VOR-DME and VORTACs) Includes VORs, VOR-DMEs, TACANs and VORTACs
    case VOR:
      if(options.isIncludedNavDbObject(atools::fs::type::VOR))
        writeVor(line, context.curFileId, false);
      break;

    case LOC: // 4 Localizer component of an ILS (Instrument Landing System)
    case LOC_ONLY: // 5 Localizer component of a localizer-only approach Includes for LDAs and SDFs
      if(options.isIncludedNavDbObject(atools::fs::type::ILS))
        writeIls(line, context.curFileId, context);
      break;

    // 6 Glideslope component of an ILS Frequency shown is paired frequency, notthe DME channel
    case GS:
      updateIlsGlideslope(line);
      break;

    // 7 Outer markers (OM) for an ILS Includes outer maker component of LOMs
    case OM:
    // 8 Middle markers (MM) for an ILS
    case MM:
    // 9 Inner markers (IM) for an ILS
    case IM:
      if(options.isIncludedNavDbObject(atools::fs::type::MARKER))
        writeMarker(line, context.curFileId, rowCode);
      break;

    // 12 DME, including the DME component of an ILS, VORTAC or VOR-DME Paired frequency display suppressed on X-Plane’s charts
    case DME:
      if(options.isIncludedNavDbObject(atools::fs::type::ILS))
      {
        if(line.last() == "DME-ILS")
          updateIlsDme(line);
      }
      break;

    // 13 Stand-alone DME, or the DME component of an NDB-DME Paired frequency will be displayed on X-Plane’s charts
    case DME_ONLY:
      if(options.isIncludedNavDbObject(atools::fs::type::ILS))
      {
        if(line.last() == "DME-ILS")
          updateIlsDme(line);
        else
        {
          writeVor(line, true, context.curFileId);
        }
      }
      break;

    case atools::fs::xp::SBAS_GBAS_FINAL:
    case atools::fs::xp::GBAS:
    case atools::fs::xp::SBAS_GBAS_TRESHOLD:
      break;
      // 14 Final approach path alignment point of an SBAS or GBAS approach path Will not appear in X-Plane’s charts
      // 15 GBAS differential ground station of a GLS Will not appear in X-Plane’s charts
      // 16 Landing threshold point or fictitious threshold point of an SBAS/GBAS approach Will not appear in X-Plane’s charts
  }
}

void XpNavWriter::finish(const XpWriterContext& context)
{
  Q_UNUSED(context);
}

void XpNavWriter::reset()
{
  airportIndex->clearSkippedIls();
}

void XpNavWriter::initQueries()
{
  deInitQueries();

  atools::sql::SqlUtil util(&db);

  insertVorQuery = new SqlQuery(db);
  insertVorQuery->prepare(util.buildInsertStatement("vor"));

  insertNdbQuery = new SqlQuery(db);
  insertNdbQuery->prepare(util.buildInsertStatement("ndb"));

  insertMarkerQuery = new SqlQuery(db);
  insertMarkerQuery->prepare(util.buildInsertStatement("marker"));

  insertIlsQuery = new SqlQuery(db);
  insertIlsQuery->prepare(util.buildInsertStatement("ils"));

  updateIlsDmeQuery = new SqlQuery(db);
  updateIlsDmeQuery->prepare("update ils set dme_range = :dme_range, dme_altitude = :dme_altitude, "
                             "dme_lonx = :dme_lonx, dme_laty = :dme_laty where ils_id = :id");

  updateIlsGsQuery = new SqlQuery(db);
  updateIlsGsQuery->prepare("update ils set gs_range = :gs_range, gs_pitch = :gs_pitch, gs_altitude = :gs_altitude, "
                            "gs_lonx = :gs_lonx, gs_laty = :gs_laty where ils_id = :id");
}

void XpNavWriter::deInitQueries()
{
  delete insertVorQuery;
  insertVorQuery = nullptr;

  delete insertNdbQuery;
  insertNdbQuery = nullptr;

  delete insertMarkerQuery;
  insertMarkerQuery = nullptr;

  delete insertIlsQuery;
  insertIlsQuery = nullptr;

  delete updateIlsDmeQuery;
  updateIlsDmeQuery = nullptr;

  delete updateIlsGsQuery;
  updateIlsGsQuery = nullptr;
}

} // namespace xp
} // namespace fs
} // namespace atools
