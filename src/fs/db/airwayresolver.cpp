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

#include "fs/db/airwayresolver.h"

#include "sql/sqldatabase.h"
#include "sql/sqlquery.h"
#include "sql/sqlutil.h"
#include "geo/pos.h"
#include "geo/rect.h"
#include "geo/calculations.h"
#include "fs/progresshandler.h"

#include <QDebug>
#include <QString>
#include <QList>
#include <algorithm>
#include <QQueue>

namespace atools {
namespace fs {
namespace db {

using atools::sql::SqlDatabase;
using atools::sql::SqlQuery;
using atools::sql::SqlUtil;
using atools::geo::Pos;
using atools::geo::Rect;

// Get all airway_point rows and join previous and next waypoints to the result by ident and region
static const QString WAYPOINT_QUERY(
  "select r.name, r.type, "
  "  prev.waypoint_id as prev_waypoint_id, "
  "  r.previous_minimum_altitude, "
  "  r.previous_maximum_altitude, "
  "  r.previous_direction, "
  "  prev.lonx as prev_lonx, "
  "  prev.laty as prev_laty, "
  "  r.waypoint_id, "
  "  w.lonx as lonx, "
  "  w.laty as laty, "
  "  next.waypoint_id as next_waypoint_id, "
  "  r.next_minimum_altitude, "
  "  r.next_maximum_altitude, "
  "  r.next_direction, "
  "  next.lonx as next_lonx, "
  "  next.laty as next_laty "
  "from airway_point r join waypoint w on r.waypoint_id = w.waypoint_id "
  "  left outer join waypoint prev on r.previous_ident = prev.ident and r.previous_region = prev.region "
  "  left outer join waypoint next on r.next_ident = next.ident and r.next_region = next.region "
  "order by r.name");

/* Airway segment with from/to position and IDs */
struct AirwayResolver::AirwaySegment
{
  AirwaySegment()
  {

  }

  AirwaySegment(int fromId, int toId, char direction, int minAltitude, int maxAltitude, QString airwayType,
                const atools::geo::Pos& fromPosition, const atools::geo::Pos& toPosition)
    : type(airwayType), dir(direction), fromWaypointId(fromId), toWaypointId(toId),
    minAlt(minAltitude), maxAlt(maxAltitude),
    fromPos(fromPosition), toPos(toPosition)
  {
  }

  bool operator==(const AirwaySegment& other) const
  {
    return fromWaypointId == other.fromWaypointId && toWaypointId == other.toWaypointId;

  }

  bool operator<(const AirwaySegment& other) const
  {
    return std::pair<int, int>(fromWaypointId, toWaypointId) <
           std::pair<int, int>(other.fromWaypointId, other.toWaypointId);
  }

  QString type;
  char dir = '\0';
  int fromWaypointId = 0, toWaypointId = 0, minAlt = 0, maxAlt;
  atools::geo::Pos fromPos, toPos;
};

uint qHash(const AirwayResolver::AirwaySegment& segment)
{
  return static_cast<unsigned int>(segment.fromWaypointId) ^
         static_cast<unsigned int>(segment.toWaypointId) ^
         static_cast<unsigned int>(segment.minAlt) ^
         static_cast<unsigned int>(segment.maxAlt) ^
         qHash(segment.type);
}

AirwayResolver::AirwayResolver(sql::SqlDatabase *sqlDb, atools::fs::ProgressHandler& progress)
  : progressHandler(progress), curAirwayId(1), numAirways(0), airwayInsertStmt(sqlDb), db(sqlDb)
{
  SqlUtil util(sqlDb);
  airwayInsertStmt.prepare(util.buildInsertStatement("airway"));
}

AirwayResolver::~AirwayResolver()
{
}

void AirwayResolver::assignWaypointIds()
{
  // Set the waypoint IDs
  SqlQuery query(db);
  query.exec(
    "update airway_point set waypoint_id = ( "
    "select w.waypoint_id from waypoint w where mid_type = w.type and mid_ident = w.ident and mid_region = w.region )");
  int updated = query.numRowsAffected();
  qInfo() << "Updated" << updated << "waypoint_id in airway table";
}

bool AirwayResolver::run()
{
  bool aborted = false;

  // Clean the table
  SqlQuery query(db);
  query.exec("delete from airway");
  int deleted = query.numRowsAffected();
  qInfo() << "Removed" << deleted << "from airway table";

  // Use set to
  QSet<AirwaySegment> airway;
  QString currentAirway;

  // Get all airway_point rows and join previous and next waypoints to the result by ident and region
  // Result is ordered by airway name
  query.exec(WAYPOINT_QUERY);
  while(query.next())
  {
    QString awName = query.value("name").toString();
    QString awType = query.value("type").toString();

    if(currentAirway.isEmpty() || awName.at(0) != currentAirway.at(0))
    {
      // Send a progress report for each airway name having a new first characters
      db->commit();
      QString msg = QString(tr("Creating airways: %1...")).arg(awName);
      qInfo() << msg;
      if((aborted = progressHandler.reportOtherMsg(msg)) == true)
        break;
    }

    if(awName != currentAirway)
    {
      // A new airway comes from from the query save the current one to the database
      if(!airway.empty())
      {
        // Build airway fragments
        QVector<Fragment> fragments;
        buildAirway(currentAirway, airway, fragments);

        // Remove all fragments that are contained by others
        cleanFragments(fragments);

        for(const Fragment& fragment : fragments)
        {
          for(const TypeRowValueVector& bindRow : fragment.boundValues)
          {
            airwayInsertStmt.bindValues(bindRow);
            airwayInsertStmt.exec();
            numAirways += airwayInsertStmt.numRowsAffected();
          }
        }
        airway.clear();
      }
      currentAirway = awName;
    }

    int currentWpId = query.value("waypoint_id").toInt();
    Pos currentWpPos(query.value("lonx").toFloat(), query.value("laty").toFloat());

    QVariant prevWpIdColVal = query.value("prev_waypoint_id");
    int prevMinAlt = query.value("previous_minimum_altitude").toInt();
    int prevMaxAlt = query.value("previous_maximum_altitude").toInt();
    char prevDir = atools::strToChar(query.value("previous_direction").toString());

    QVariant nextWpIdColVal = query.value("next_waypoint_id");
    int nextMinAlt = query.value("next_minimum_altitude").toInt();
    int nextMaxAlt = query.value("next_maximum_altitude").toInt();
    char nextDir = atools::strToChar(query.value("next_direction").toString());

    if(!prevWpIdColVal.isNull())
    {
      // Previous waypoint found - add segment
      Pos prevPos(query.value("prev_lonx").toFloat(), query.value("prev_laty").toFloat());

      if(currentWpPos.distanceMeterTo(prevPos) < atools::geo::nmToMeter(maxAirwaySegmentLength))
        airway.insert(AirwaySegment(prevWpIdColVal.toInt(), currentWpId, prevDir, prevMinAlt, prevMaxAlt, awType,
                                    prevPos, currentWpPos));
    }

    if(!nextWpIdColVal.isNull())
    {
      // Next waypoint found - add segment
      Pos nextPos(query.value("next_lonx").toFloat(), query.value("next_laty").toFloat());

      if(currentWpPos.distanceMeterTo(nextPos) < atools::geo::nmToMeter(maxAirwaySegmentLength))
        airway.insert(AirwaySegment(currentWpId, nextWpIdColVal.toInt(), nextDir, nextMinAlt, nextMaxAlt, awType,
                                    currentWpPos, nextPos));
    }
  }

  qInfo() << "Added " << numAirways << " airway segments";

  if(!aborted)
    db->commit();

  return aborted;
}

void AirwayResolver::buildAirway(const QString& airwayName, QSet<AirwaySegment>& airway,
                                 QVector<Fragment>& fragments)
{
  // Queue of waypoints that will get waypoints in order prependend and appendend
  QQueue<AirwaySegment> newAirway;

  // Segments indexed by from waypoint ID
  QHash<int, AirwaySegment> segsByFromWpId;
  // Segments indexed by to waypoint ID
  QHash<int, AirwaySegment> segsByToWpId;

  // Fill the index
  for(const AirwaySegment& segment : airway)
  {
    segsByFromWpId[segment.fromWaypointId] = segment;
    segsByToWpId[segment.toWaypointId] = segment;
  }

  int fragmentNum = 1;
  QHash<int, AirwaySegment>::const_iterator it;
  AirwaySegment segment;

  // Iterator over all waypoints in the airway which are neither ordered nor connected yet
  // All waypoints in airway have same airway name
  while(!airway.empty())
  {
    newAirway.clear();

    // Take a random waypoint from the unordered airway and add it to the queue
    segment = *airway.begin();
    airway.erase(airway.begin());
    newAirway.append(segment);

    bool foundTo, foundFrom;

    // Now collect predecessors and successors for all waypoints
    do
    {
      foundTo = false;
      foundFrom = false;

      // Take a segment from the front of the queue and find predecessors
      segment = newAirway.front();
      it = segsByToWpId.find(segment.fromWaypointId);
      if(it != segsByToWpId.end() && airway.find(it.value()) != airway.end())
      {
        // Found a predecessor in the index - add it to the new airway and remove it from the queue
        segment = it.value();
        newAirway.prepend(segment);

        airway.erase(airway.find(segment));
        foundTo = true;
      }

      // Take a segment from the end of the queue and find successors
      segment = newAirway.back();
      it = segsByFromWpId.find(segment.toWaypointId);
      if(it != segsByFromWpId.end() && airway.find(it.value()) != airway.end())
      {
        // Found a successor in the index - add it to the new airway and remove it from the queue
        segment = it.value();
        newAirway.append(segment);

        airway.erase(airway.find(segment));
        foundFrom = true;
      }
    } while(foundTo || foundFrom);

    // Write airway fragment - there may be more fragments for the same airway name
    AirwaySegment last;
    int seqNo = 1;
    Fragment fragment;

    for(const AirwaySegment& newSegment : newAirway)
    {
      last = newSegment;

      fragment.waypoints.insert(newSegment.fromWaypointId);
      fragment.waypoints.insert(newSegment.toWaypointId);

      // Create bounding rect for this segment
      Rect bounding(newSegment.fromPos);
      bounding.extend(newSegment.toPos);

      TypeRowValueVector row;

      row.append(std::make_pair(":airway_id", curAirwayId));
      row.append(std::make_pair(":airway_name", airwayName));
      row.append(std::make_pair(":airway_type", newSegment.type));
      row.append(std::make_pair(":airway_fragment_no", fragmentNum));
      row.append(std::make_pair(":sequence_no", seqNo));

      row.append(std::make_pair(":from_waypoint_id", newSegment.fromWaypointId));
      row.append(std::make_pair(":to_waypoint_id", newSegment.toWaypointId));

      row.append(std::make_pair(":direction", atools::charToStr(newSegment.dir)));
      row.append(std::make_pair(":minimum_altitude", newSegment.minAlt));
      row.append(std::make_pair(":maximum_altitude", newSegment.maxAlt));
      row.append(std::make_pair(":left_lonx", bounding.getTopLeft().getLonX()));
      row.append(std::make_pair(":top_laty", bounding.getTopLeft().getLatY()));
      row.append(std::make_pair(":right_lonx", bounding.getBottomRight().getLonX()));
      row.append(std::make_pair(":bottom_laty", bounding.getBottomRight().getLatY()));

      // Write start and end coordinates for this segment
      row.append(std::make_pair(":from_lonx", newSegment.fromPos.getLonX()));
      row.append(std::make_pair(":from_laty", newSegment.fromPos.getLatY()));
      row.append(std::make_pair(":to_lonx", newSegment.toPos.getLonX()));
      row.append(std::make_pair(":to_laty", newSegment.toPos.getLatY()));

      fragment.boundValues.append(row);

      seqNo++;
      curAirwayId++;
    }
    fragments.append(fragment);

    fragmentNum++;
  }
}

void AirwayResolver::cleanFragments(QVector<Fragment>& fragments)
{
  // Erase empty segments
  auto it = std::remove_if(fragments.begin(), fragments.end(), [](const Fragment& f) -> bool
        {
          return f.waypoints.size() < 2;
        });
  if(it != fragments.end())
    fragments.erase(it, fragments.end());

  // Erase all segments that are contained by another
  for(int i = 0; i < fragments.size(); i++)
  {
    Fragment& f1 = fragments[i];
    for(int j = 0; j < fragments.size(); j++)
    {
      if(j == i)
        continue;

      Fragment& f2 = fragments[j];

      if(!f2.waypoints.isEmpty() && f1.waypoints.contains(f2.waypoints))
        f2.waypoints.clear();
    }
  }

  // Remove the marked segments
  auto it2 = std::remove_if(fragments.begin(), fragments.end(), [](const Fragment& f) -> bool
        {
          return f.waypoints.isEmpty();
        });
  if(it2 != fragments.end())
    fragments.erase(it2, fragments.end());
}

} // namespace writer
} // namespace fs
} // namespace atools
