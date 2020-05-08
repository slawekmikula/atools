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

#ifndef ATOOLS_FS_DB_AP_DELETEPROCESSOR_H
#define ATOOLS_FS_DB_AP_DELETEPROCESSOR_H

#include "fs/bgl/ap/airport.h"

namespace bgl {
namespace ap {
class Airport;
namespace del {
class DeleteAirport;
}
}
}

namespace atools {
namespace sql {
class SqlQuery;
class SqlDatabase;
}
namespace fs {
namespace db {

class DataWriter;
class ApproachWriter;

/*
 * Deletes stock/default airports for a new airport. Uses the delete records and removes or updates all
 * old airports and their facilities.
 */
class DeleteProcessor
{
public:
  DeleteProcessor(atools::sql::SqlDatabase& sqlDb, const atools::fs::NavDatabaseOptions& opts);
  virtual ~DeleteProcessor();

  /*
   * Initialize the process for one airport before it is stored in the database.
   */
  void init(const atools::fs::bgl::DeleteAirport *deleteAirportRec,
            const atools::fs::bgl::Airport *airport, int airportId);

  /*
   * Start the update or removal process of airports before the new airport is stored in the databas.
   */
  void preProcessDelete();

  /*
   * Start the update or removal process of airports. The current/new airport has to
   * be stored in the database already.
   */
  void postProcessDelete();

  const QString& getBglFilename() const
  {
    return bglFilename;
  }

  const QString& getSceneryLocalPath() const
  {
    return sceneryLocalPath;
  }

private:
  int executeStatement(sql::SqlQuery *stmt, const QString& what);
  void fetchIds(sql::SqlQuery *stmt, QList<int>& ids, const QString& what);

  void removeRunways();
  void removeAirport();

  QString updateAptFeatureStmt(const QString& table);
  QString delAptFeatureStmt(const QString& table);
  void removeOrUpdate(sql::SqlQuery *deleteStmt, sql::SqlQuery *updateStmt,
                      atools::fs::bgl::del::DeleteAllFlags flag);
  QString updateAptFeatureToNullStmt(const QString& table);
  void removeApproachesAndTransitions(const QList<int>& ids);
  void extractDeleteFlags();

  int bindAndExecute(sql::SqlQuery *query, const QString& msg);
  int bindAndExecute(const QString& sql, const QString& msg);
  void extractPreviousAirportFeatures();
  void copyAirportValues(const QStringList& copyAirportColumns);
  void updateBoundingRect();

  const atools::fs::NavDatabaseOptions& options;

  atools::sql::SqlQuery
  *deleteRunwayStmt = nullptr,
  *updateRunwayStmt = nullptr,
  *deleteParkingStmt = nullptr,
  *updateParkingStmt = nullptr,
  *deleteDeleteApStmt = nullptr,
  *fetchRunwayEndIdStmt = nullptr,
  *updateApprochRwIds = nullptr,
  *deleteRunwayEndStmt = nullptr,
  *updateWpStmt = nullptr,
  *updateVorStmt = nullptr,
  *updateNdbStmt = nullptr,
  *deleteApproachStmt = nullptr, *updateApproachStmt = nullptr,
  *deleteAirportStmt = nullptr, *selectAirportStmt = nullptr,
  *deleteApronStmt = nullptr, *updateApronStmt = nullptr,
  *deleteApronLightStmt = nullptr, *updateApronLightStmt = nullptr,
  *deleteFenceStmt = nullptr, *updateFenceStmt = nullptr,
  *deleteHelipadStmt = nullptr, *updateHelipadStmt = nullptr,
  *deleteStartStmt = nullptr, *updateStartStmt = nullptr,
  *deleteTaxiPathStmt = nullptr, *updateTaxiPathStmt = nullptr,
  *deleteComStmt = nullptr, *updateComStmt = nullptr,
  *fetchPrimaryAppStmt = nullptr, *fetchSecondaryAppStmt = nullptr,
  *updateBoundingStmt = nullptr, *fetchBoundingStmt = nullptr;

  const atools::fs::bgl::DeleteAirport *deleteAirport = nullptr;
  atools::fs::bgl::del::DeleteAllFlags deleteFlags = atools::fs::bgl::del::NONE;
  const atools::fs::bgl::Airport *newAirport = nullptr;
  int currentAirportId = 0;
  QString ident, bglFilename, sceneryLocalPath;
  atools::sql::SqlDatabase *db = nullptr;

  bool prevHasApproach = false, prevHasApron = false, prevHasCom = false, prevHasHelipad = false,
       prevHasTaxi = false, prevHasStart = false, prevHasRunways = false, isAddon = false;
  int previousRating = 0;
  bool hasPrevious = false;
  int prevAirportId = 0;
  atools::geo::Pos prevPos;

};

} // namespace writer
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_DB_AP_DELETEPROCESSOR_H
