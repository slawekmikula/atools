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

#include "fs/navdatabase.h"
#include "sql/sqldatabase.h"
#include "sql/sqlscript.h"
#include "fs/db/datawriter.h"
#include "sql/sqlutil.h"
#include "sql/sqltransaction.h"
#include "fs/scenery/scenerycfg.h"
#include "fs/scenery/addoncfg.h"
#include "fs/db/airwayresolver.h"
#include "fs/db/routeedgewriter.h"
#include "fs/progresshandler.h"
#include "fs/scenery/fileresolver.h"
#include "fs/scenery/addonpackage.h"
#include "fs/xp/xpdatacompiler.h"
#include "fs/dfd/dfdcompiler.h"
#include "fs/db/databasemeta.h"
#include "atools.h"
#include "exception.h"

#include <QDir>
#include <QElapsedTimer>
#include <QStandardPaths>

namespace atools {
namespace fs {

// Number of progress steps besides scenery areas
// Database report steps

// Number of steps for general tasks - increase > 1 to make them more visible in progress
static const int PROGRESS_NUM_TASK_STEPS = 10;

// runScript()
static const int PROGRESS_NUM_SCRIPT_STEPS = PROGRESS_NUM_TASK_STEPS;

// AirwayResolver steps - larger number makes task take more time of progress bar
static const int PROGRESS_NUM_RESOLVE_AIRWAY_STEPS = 1000;

// createSchemaInternal()
static const int PROGRESS_NUM_SCHEMA_STEPS = 8;

const static QChar SEP(QDir::separator());

using atools::sql::SqlScript;
using atools::sql::SqlQuery;
using atools::sql::SqlUtil;
using atools::sql::SqlTransaction;
using atools::fs::scenery::SceneryCfg;
using atools::fs::scenery::AddOnCfg;
using atools::fs::scenery::AddOnCfgEntry;
using atools::fs::scenery::SceneryArea;
using atools::fs::scenery::AddOnComponent;
using atools::fs::scenery::AddOnPackage;

NavDatabase::NavDatabase(const NavDatabaseOptions *readerOptions, sql::SqlDatabase *sqlDb,
                         NavDatabaseErrors *databaseErrors, const QString& revision)
  : db(sqlDb), errors(databaseErrors), options(readerOptions), gitRevision(revision)
{

}

void NavDatabase::create(const QString& codec)
{
  if(options != nullptr)
    qDebug() << Q_FUNC_INFO << *options;

  createInternal(codec);
  if(aborted)
    // Remove all (partial) changes
    db->rollback();
}

void NavDatabase::createAirspaceSchema()
{
  SqlScript script(db, true /* options->isVerbose()*/);
  script.executeScript(":/atools/resources/sql/fs/db/drop_meta.sql");
  script.executeScript(":/atools/resources/sql/fs/db/drop_nav.sql");
  script.executeScript(":/atools/resources/sql/fs/db/create_boundary_schema.sql");
  script.executeScript(":/atools/resources/sql/fs/db/create_meta_schema.sql");
  script.executeScript(":/atools/resources/sql/fs/db/create_indexes_post_load_boundary.sql");
}

void NavDatabase::createSchema()
{
  createSchemaInternal(nullptr);
}

void NavDatabase::createSchemaInternal(ProgressHandler *progress)
{
  SqlTransaction transaction(db);

  SqlScript script(db, true /* options->isVerbose()*/);

  if(progress != nullptr)
    if((aborted = progress->reportOther(tr("Removing Views"))))
      return;

  script.executeScript(":/atools/resources/sql/fs/db/drop_view.sql");

  if(progress != nullptr)
    if((aborted = progress->reportOther(tr("Removing Routing and Search"))))
      return;

  script.executeScript(":/atools/resources/sql/fs/db/drop_routing_search.sql");

  if(progress != nullptr)
    if((aborted = progress->reportOther(tr("Removing Navigation Aids"))))
      return;

  script.executeScript(":/atools/resources/sql/fs/db/drop_nav.sql");

  if(progress != nullptr)
    if((aborted = progress->reportOther(tr("Removing Airport Facilites"))))
      return;

  script.executeScript(":/atools/resources/sql/fs/db/drop_airport_facilities.sql");

  if(progress != nullptr)
    if((aborted = progress->reportOther(tr("Removing Approaches"))))
      return;

  script.executeScript(":/atools/resources/sql/fs/db/drop_approach.sql");

  if(progress != nullptr)
    if((aborted = progress->reportOther(tr("Removing Airports"))))
      return;

  script.executeScript(":/atools/resources/sql/fs/db/drop_airport.sql");

  if(progress != nullptr)
    if((aborted = progress->reportOther(tr("Removing Metadata"))))
      return;

  script.executeScript(":/atools/resources/sql/fs/db/drop_meta.sql");
  transaction.commit();

  if(progress != nullptr)
    if((aborted = progress->reportOther(tr("Creating Database Schema"))))
      return;

  script.executeScript(":/atools/resources/sql/fs/db/create_boundary_schema.sql");
  script.executeScript(":/atools/resources/sql/fs/db/create_nav_schema.sql");
  script.executeScript(":/atools/resources/sql/fs/db/create_ap_schema.sql");
  // if(options->isCreateRouteTables())
  script.executeScript(":/atools/resources/sql/fs/db/create_route_schema.sql");
  script.executeScript(":/atools/resources/sql/fs/db/create_meta_schema.sql");
  script.executeScript(":/atools/resources/sql/fs/db/create_views.sql");
  transaction.commit();
}

bool NavDatabase::isSceneryConfigValid(const QString& filename, const QString& codec, QString& error)
{
  QFileInfo fi(filename);
  if(fi.exists())
  {
    if(fi.isReadable())
    {
      if(fi.isFile())
      {
        try
        {
          // Read the scenery file and check if it has at least one scenery area
          SceneryCfg cfg(codec);
          cfg.read(filename);

          return !cfg.getAreas().isEmpty();
        }
        catch(atools::Exception& e)
        {
          qWarning() << "Caught exception reading" << filename << ":" << e.what();
          error = e.what();
        }
        catch(...)
        {
          qWarning() << "Caught unknown exception reading" << filename;
          error = "Unknown exception while reading file";
        }
      }
      else
        error = tr("File is not a regular file");
    }
    else
      error = tr("File is not readable");
  }
  else
    error = tr("File does not exist");
  return false;
}

bool NavDatabase::isBasePathValid(const QString& filepath, QString& error, atools::fs::FsPaths::SimulatorType type)
{
  QFileInfo fi(filepath);
  if(fi.exists())
  {
    if(fi.isReadable())
    {
      if(fi.isDir())
      {
        if(type == atools::fs::FsPaths::XPLANE11)
        {
          QFileInfo dataDir(filepath + SEP + "Resources" + SEP + "default data");

          if(dataDir.exists() && dataDir.isDir() && dataDir.isReadable())
            return true;
          else
            error = tr("\"%1\" not found").arg(QString("Resources") + SEP + "default data");
        }
        else
        {
          // If path exists check for scenery directory
          QDir dir(filepath);
          QFileInfoList scenery = dir.entryInfoList({"scenery"}, QDir::Dirs);
          if(!scenery.isEmpty())
            return true;
          else
            error = tr("Does not contain a \"Scenery\" directory");
        }
      }
      else
        error = tr("Is not a directory");
    }
    else
      error = tr("Directory is not readable");
  }
  else
    error = tr("Directory does not exist");
  return false;
}

// X-Plane steps ========================================================================================
// =P=== Total Progress 5604
// =P=== "1 of 5604 (0 %) [1]" "Removing Views"
// =P=== "2 of 5604 (0 %) [1]" "Removing Routing and Search"
// =P=== "3 of 5604 (0 %) [1]" "Removing Navigation Aids"
// =P=== "4 of 5604 (0 %) [1]" "Removing Airport Facilites"
// =P=== "5 of 5604 (0 %) [1]" "Removing Approaches"
// =P=== "6 of 5604 (0 %) [1]" "Removing Airports"
// =P=== "7 of 5604 (0 %) [1]" "Removing Metadata"
// =P=== "8 of 5604 (0 %) [1]" "Creating Database Schema"
// =P=====================================================================
// =P=== "9 of 5604 (0 %) [1]" "X-Plane"
// =P=== ""
// "/home/alex/Daten/Programme/X-Plane 11/Custom Scenery/XXXXXXXXX"
// =P=== "2382 of 5604 (42 %) [10]" "Creating indexes"
// =P=== "2392 of 5604 (42 %) [10]" "Creating boundary indexes"
// =P=== "2403 of 5604 (42 %) [10]" "Clean up"
// =P=== "2513 of 5604 (44 %) [10]" "Preparing Airways"
// =P=== "2514 of 5604 (44 %) [1]" "Post procecssing Airways"
// "/home/alex/Daten/Programme/X-Plane 11/Custom Data/CIFP/XXXXXXXXXXX"
// =P=== "4602 of 5604 (82 %) [1]" "Creating airways: B953..."
// =P=== "4751 of 5604 (84 %) [1]" "Creating airways: M611..."
// =P=== "4900 of 5604 (87 %) [1]" "Creating airways: T317..."
// =P=== "5050 of 5604 (90 %) [1]" "Creating airways: UR544..."
// =P=== "5198 of 5604 (92 %) [1]" "Creating airways: V37..."
// =P=== "5343 of 5604 (95 %) [1]" "Creating airways: Y336..."
// =P=== "5525 of 5604 (98 %) [10]" "Updating waypoints"
// =P=== "5535 of 5604 (98 %) [10]" "Updating approaches"
// =P=== "5545 of 5604 (98 %) [10]" "Updating Airports"
// =P=== "5555 of 5604 (99 %) [10]" "Updating ILS Count"
// =P=== "5565 of 5604 (99 %) [10]" "Collecting navaids for search"
// =P=== "5575 of 5604 (99 %) [10]" "Creating indexes for airport"
// =P=== "5585 of 5604 (99 %) [10]" "Creating indexes for search"
// =P=== "5595 of 5604 (99 %) [10]" "Vacuum Database"
// =P=== "5604 of 5604 (100 %) [10]" "Analyze Database"
int NavDatabase::countXplaneSteps()
{
  // Create schema "Removing Views" ... "Creating Database Schema"
  int total = PROGRESS_NUM_SCHEMA_STEPS;
  total++; // Scenery "X-Plane"
  total += atools::fs::xp::XpDataCompiler::calculateReportCount(*options); // All files
  total += PROGRESS_NUM_TASK_STEPS; // "Creating indexes"
  total += PROGRESS_NUM_TASK_STEPS; // "Creating boundary indexes"
  if(options->isDeduplicate())
    total += PROGRESS_NUM_TASK_STEPS; // "Clean up"
  total += PROGRESS_NUM_TASK_STEPS; // "Preparing Airways"
  total++; // "Post procecssing Airways" (XpDataCompiler)
  if(options->isResolveAirways())
    total += PROGRESS_NUM_RESOLVE_AIRWAY_STEPS; // "Creating airways"
  total += PROGRESS_NUM_TASK_STEPS; // "Updating waypoints"
  total += PROGRESS_NUM_TASK_STEPS; // "Updating approaches"
  total += PROGRESS_NUM_TASK_STEPS; // "Updating Airports"
  total += PROGRESS_NUM_TASK_STEPS; // "Updating ILS Count"
  total += PROGRESS_NUM_TASK_STEPS; // "Collecting navaids for search"
  total += PROGRESS_NUM_TASK_STEPS; // "Creating indexes for airport"
  total += PROGRESS_NUM_TASK_STEPS; // "Creating indexes for search"
  if(options->isVacuumDatabase())
    total += PROGRESS_NUM_TASK_STEPS; // "Vacuum Database"
  if(options->isAnalyzeDatabase())
    total += PROGRESS_NUM_TASK_STEPS; // "Analyze Database"

  // Not used in production
  // if(options->isDatabaseReport())
  // if(options->isDropIndexes())
  return total;
}

// DFD steps ========================================================================================
// void atools::fs::NavDatabase::createInternal(const QString&) =P=== progress total 1164
// =P=== "1 of 1164 (0 %)" "Removing Views"
// =P=== "2 of 1164 (0 %)" "Removing Routing and Search"
// =P=== "3 of 1164 (0 %)" "Removing Navigation Aids"
// =P=== "4 of 1164 (0 %)" "Removing Airport Facilites"
// =P=== "5 of 1164 (0 %)" "Removing Approaches"
// =P=== "6 of 1164 (0 %)" "Removing Airports"
// =P=== "7 of 1164 (0 %)" "Removing Metadata"
// =P=== "8 of 1164 (0 %)" "Creating Database Schema"
// =P=====================================================================
// =P=== "9 of 1164 (0 %)" "Navigraph"
// =P=== ""
// =P=== "10 of 1164 (0 %)" "Writing MORA"
// =P=== "11 of 1164 (0 %)" "Writing airports"
// =P=== "12 of 1164 (1 %)" "Writing runways"
// =P=== "13 of 1164 (1 %)" "Writing navaids"
// =P=== "14 of 1164 (1 %)" "Writing Airspaces"
// =P=== "15 of 1164 (1 %)" "Writing Airspaces COM"
// =P=== "16 of 1164 (1 %)" "Writing COM Frequencies"
// =P=== "26 of 1164 (2 %)" "Creating indexes"
// =P=== "36 of 1164 (3 %)" "Creating boundary indexes"
// =P=== "46 of 1164 (3 %)" "Clean up"
// =P=== "47 of 1164 (4 %)" "Writing airways"
// =P=== "57 of 1164 (4 %)" "Creating waypoints for procedures"
// =P=== "58 of 1164 (4 %)" "Updating magnetic declination"
// =P=== "59 of 1164 (5 %)" "Updating VORTAC and TACAN channels"
// =P=== "60 of 1164 (5 %)" "Updating ILS geometry"
// =P=== "61 of 1164 (5 %)" "Writing approaches and transitions"
// =P=== "62 of 1164 (5 %)" "Writing SIDs"
// =P=== "63 of 1164 (5 %)" "Writing STARs"
// =P=== "73 of 1164 (6 %)" "Creating indexes"
// =P=== "83 of 1164 (7 %)" "Creating boundary indexes"
// =P=== "93 of 1164 (7 %)" "Updating Navids in Waypoint"
// =P=== "94 of 1164 (8 %)" "Updating airport idents"
// =P=== "104 of 1164 (8 %)" "Updating waypoints"
// =P=== "114 of 1164 (9 %)" "Merging VOR and TACAN to VORTAC"
// =P=== "124 of 1164 (10 %)" "Updating approaches"
// =P=== "134 of 1164 (11 %)" "Updating Airports"
// =P=== "144 of 1164 (12 %)" "Updating ILS"
// =P=== "154 of 1164 (13 %)" "Updating ILS Count"
// =P=== "164 of 1164 (14 %)" "Collecting navaids for search"
// =P=== "174 of 1164 (14 %)" "Populating routing tables"
// =P=== "175 of 1164 (15 %)" "Creating route edges for VOR and NDB"
// =P=== "185 of 1164 (15 %)" "Creating route edges waypoints"
// =P=== "195 of 1164 (16 %)" "Creating indexes for airport"
// =P=== "205 of 1164 (17 %)" "Creating indexes for search"
// =P=== "215 of 1164 (18 %)" "Creating indexes for route"
// =P=== "216 of 1164 (18 %)" "Basic Validation"
// =P=== "217 of 1164 (18 %)" "Creating table statistics"
// =P=== "218 of 1164 (18 %)" "Creating report on values"
// =P=== "219 of 1164 (18 %)" "Creating report on duplicates"
// =P=== "220 of 1164 (18 %)" "Creating report on coordinate duplicates"
// =P=== "221 of 1164 (18 %)" "Creating Database preparation Script"
// =P=== "222 of 1164 (19 %)" "Dropping All Indexes"
// =P=== "232 of 1164 (19 %)" "Vacuum Database"
// =P=== "242 of 1164 (20 %)" "Analyze Database"
int NavDatabase::countDfdSteps()
{
  // Create schema "Removing Views" ... "Creating Database Schema"
  int total = PROGRESS_NUM_SCHEMA_STEPS;
  total++; // Scenery "Navigraph"
  total++; // "Writing MORA"
  total++; // "Writing airports"
  total++; // "Writing runways"
  total++; // "Writing navaids"
  total++; // "Writing Airspaces"
  total++; // "Writing Airspaces COM"
  total++; // "Writing COM Frequencies"
  total += PROGRESS_NUM_TASK_STEPS; // "Creating indexes"
  total += PROGRESS_NUM_TASK_STEPS; // "Creating boundary indexes"
  if(options->isDeduplicate())
    total += PROGRESS_NUM_TASK_STEPS; // "Clean up"
  total++; // "Writing airways"
  total += PROGRESS_NUM_TASK_STEPS; // "Creating waypoints for procedures"
  total++; // "Updating magnetic declination"
  total++; // "Updating VORTAC and TACAN channels"
  total++; // "Updating ILS geometry"
  total++; // "Writing approaches and transitions"
  total++; // "Writing SIDs"
  total++; // "Writing STARs"
  total += PROGRESS_NUM_TASK_STEPS; // "Creating indexes"
  total += PROGRESS_NUM_TASK_STEPS; // "Creating boundary indexes"
  total += PROGRESS_NUM_TASK_STEPS; // "Updating Navids in Waypoint"
  total++; // "Updating airport idents"
  total += PROGRESS_NUM_TASK_STEPS; // "Updating waypoints"
  total += PROGRESS_NUM_TASK_STEPS; // "Merging VOR and TACAN to VORTAC"
  total += PROGRESS_NUM_TASK_STEPS; // "Updating approaches"
  total += PROGRESS_NUM_TASK_STEPS; // "Updating Airports"
  total += PROGRESS_NUM_TASK_STEPS; // "Updating ILS"
  total += PROGRESS_NUM_TASK_STEPS; // "Updating ILS Count"
  total += PROGRESS_NUM_TASK_STEPS; // "Collecting navaids for search"

  if(options->isCreateRouteTables())
  {
    total++; // "Populating routing tables"
    total += PROGRESS_NUM_TASK_STEPS; // "Creating route edges for VOR and NDB"
    total += PROGRESS_NUM_TASK_STEPS; // "Creating route edges waypoints"
  }
  total += PROGRESS_NUM_TASK_STEPS; // "Creating indexes for airport"
  total += PROGRESS_NUM_TASK_STEPS; // "Creating indexes for search"
  total++; // "Creating indexes for route"
  if(options->isDatabaseReport())
    // "Basic Validation"
    // "Creating table statistics" "Creating report on values" "Creating report on duplicates"
    // "Creating report on coordinate duplicates"
    total += PROGRESS_NUM_TASK_STEPS;

  if(options->isDropIndexes())
  {
    total++; // "Creating Database preparation Script"
    total++; // "Dropping All Indexes"
  }

  // "Vacuum Database"
  if(options->isVacuumDatabase())
    total += PROGRESS_NUM_TASK_STEPS;

  // "Analyze Database"
  if(options->isAnalyzeDatabase())
    total += PROGRESS_NUM_TASK_STEPS;

  total += 4; // Correction value

  return total;
}

// FSX/P3D steps ========================================================================================
// =P=== "1 of 3101 (0 %) [1]" "Removing Views"
// =P=== "2 of 3101 (0 %) [1]" "Removing Routing and Search"
// =P=== "3 of 3101 (0 %) [1]" "Removing Navigation Aids"
// =P=== "4 of 3101 (0 %) [1]" "Removing Airport Facilites"
// =P=== "5 of 3101 (0 %) [1]" "Removing Approaches"
// =P=== "6 of 3101 (0 %) [1]" "Removing Airports"
// =P=== "7 of 3101 (0 %) [1]" "Removing Metadata"
// =P=== "8 of 3101 (0 %) [1]" "Creating Database Schema"
// =P=====================================================================
// =P=== "9 of 3101 (0 %) [1]" "Default Terrain"
// =P=== "Scenery/World"
// ...
// =P=====================================================================
// =P=== "1969 of 3101 (63 %) [1]" "Addon Scenery"
// =P=== "Addon Scenery"
// =P=== "1979 of 3101 (63 %) [10]" "Creating indexes"
// =P=== "1989 of 3101 (64 %) [10]" "Creating boundary indexes"
// =P=== "1999 of 3101 (64 %) [10]" "Clean up"
// =P=== "2361 of 3101 (76 %) [1]" "Creating airways: R210..."
// =P=== "2943 of 3101 (94 %) [1]" "Creating airways: W5..."
// =P=== "3010 of 3101 (97 %) [10]" "Merging VOR and TACAN to VORTAC"
// =P=== "3020 of 3101 (97 %) [10]" "Updating waypoints"
// =P=== "3030 of 3101 (97 %) [10]" "Updating approaches"
// =P=== "3040 of 3101 (98 %) [10]" "Updating Airports"
// =P=== "3050 of 3101 (98 %) [10]" "Updating ILS"
// =P=== "3060 of 3101 (98 %) [10]" "Updating ILS Count"
// =P=== "3070 of 3101 (99 %) [10]" "Collecting navaids for search"
// =P=== "3080 of 3101 (99 %) [10]" "Creating indexes for airport"
// =P=== "3090 of 3101 (99 %) [10]" "Clean up runways"
// =P=== "3100 of 3101 (99 %) [10]" "Creating indexes for search"
// =P=== "3101 of 3101 (100 %) [10]" "Vacuum Database"
// =P=== "3101 of 3101 (100 %) [10]" "Analyze Database"
int NavDatabase::countFsxP3dSteps(const SceneryCfg& cfg)
{
  // Count the files for exact progress reporting
  int numProgressReports = 0, numSceneryAreas = 0;
  countFiles(cfg, &numProgressReports, &numSceneryAreas);

  qDebug() << Q_FUNC_INFO << "=P=== FSX/P3D files" << numProgressReports << "scenery areas" << numSceneryAreas;

  // PROGRESS_NUM_SCHEMA_STEPS Create schema "Removing Views" ... "Creating Database Schema"
  int total = numProgressReports + numSceneryAreas + PROGRESS_NUM_SCHEMA_STEPS;

  total += PROGRESS_NUM_TASK_STEPS; // "Creating indexes"
  total += PROGRESS_NUM_TASK_STEPS; // "Creating boundary indexes"
  if(options->isDeduplicate())
    total += PROGRESS_NUM_TASK_STEPS; // "Clean up"
  if(options->isResolveAirways())
    total += PROGRESS_NUM_RESOLVE_AIRWAY_STEPS; // "Creating airways"
  total += PROGRESS_NUM_TASK_STEPS; // "Merging VOR and TACAN to VORTAC"
  total += PROGRESS_NUM_TASK_STEPS; // "Updating waypoints"
  total += PROGRESS_NUM_TASK_STEPS; // "Updating approaches"
  total += PROGRESS_NUM_TASK_STEPS; // "Updating Airports"
  total += PROGRESS_NUM_TASK_STEPS; // "Updating ILS"
  total += PROGRESS_NUM_TASK_STEPS; // "Updating ILS Count"
  total += PROGRESS_NUM_TASK_STEPS; // "Collecting navaids for search"
  total += PROGRESS_NUM_TASK_STEPS; // "Creating indexes for airport"
  total += PROGRESS_NUM_TASK_STEPS; // "Clean up runways"
  total += PROGRESS_NUM_TASK_STEPS; // "Creating indexes for search"
  if(options->isVacuumDatabase())
    total += PROGRESS_NUM_TASK_STEPS; // "Vacuum Database"
  if(options->isAnalyzeDatabase())
    total += PROGRESS_NUM_TASK_STEPS; // "Analyze Database"

  // Not used in production
  // if(options->isDatabaseReport())
  // if(options->isDropIndexes())

  return total;
}

void NavDatabase::createInternal(const QString& sceneryConfigCodec)
{
  SceneryCfg cfg(sceneryConfigCodec);

  QElapsedTimer timer;
  timer.start();

  FsPaths::SimulatorType sim = options->getSimulatorType();

  if(options->isAutocommit())
    db->setAutocommit(true);

  // ==============================================================================
  // Calculate the total number of progress steps
  int total = 0;
  if(sim == atools::fs::FsPaths::XPLANE11)
    total = countXplaneSteps();
  else if(sim == atools::fs::FsPaths::NAVIGRAPH)
    total = countDfdSteps();
  else
  {
    // Read scenery.cfg
    readSceneryConfig(cfg);

    total = countFsxP3dSteps(cfg);
  }

  qDebug() << "=P=== Total Progress" << total;

  ProgressHandler progress(options);
  progress.setTotal(total);

  createSchemaInternal(&progress);
  if(aborted)
    return;

  // -----------------------------------------------------------------------
  // Create empty data writer pointers which will read all files and fill the database
  // Pointers will be initialized on demand/compilation type
  QScopedPointer<atools::fs::db::DataWriter> fsDataWriter;
  QScopedPointer<atools::fs::xp::XpDataCompiler> xpDataCompiler;
  QScopedPointer<atools::fs::ng::DfdCompiler> dfdCompiler;

  // ================================================================================================
  // Start compilation
  if(sim == atools::fs::FsPaths::NAVIGRAPH)
  {
    // Create a single Navigraph scenery area
    atools::fs::scenery::SceneryArea area(1, 1, tr("Navigraph"), QString());

    // Prepare error collection for single area
    if(errors != nullptr)
      errors->init(area);

    // Load Navigraph from source database ======================================================
    dfdCompiler.reset(new atools::fs::ng::DfdCompiler(*db, *options, &progress, errors));
    loadDfd(&progress, dfdCompiler.data(), area);
    dfdCompiler->close();
  }
  else if(sim == atools::fs::FsPaths::XPLANE11)
  {
    // Create a single X-Plane scenery area
    atools::fs::scenery::SceneryArea area(1, 1, tr("X-Plane"), QString());

    // Prepare error collection for single area
    if(errors != nullptr)
      errors->init(area);

    // Load X-Plane scenery database ======================================================
    xpDataCompiler.reset(new atools::fs::xp::XpDataCompiler(*db, *options, &progress, errors));
    loadXplane(&progress, xpDataCompiler.data(), area);
    xpDataCompiler->close();
  }
  else
  {
    // Load FSX / P3D scenery database ======================================================
    fsDataWriter.reset(new atools::fs::db::DataWriter(*db, *options, &progress));
    loadFsxP3d(&progress, fsDataWriter.data(), cfg);
    fsDataWriter->close();
  }

  if(aborted)
    return;

  // ===========================================================================
  // Loading is done here - now continue with the post process steps

  if(options->isResolveAirways() && sim != atools::fs::FsPaths::NAVIGRAPH)
  {
    // Read airway_point table, connect all waypoints and write the ordered result into the airway table
    atools::fs::db::AirwayResolver resolver(db, progress);

    if(sim != atools::fs::FsPaths::NAVIGRAPH && sim != atools::fs::FsPaths::XPLANE11)
      // Drop large segments only for FSX/P3D - default is 1000 nm
      resolver.setMaxAirwaySegmentLength(20000);

    resolver.assignWaypointIds();

    if((aborted = resolver.run(PROGRESS_NUM_RESOLVE_AIRWAY_STEPS)))
      return;
  }

  if(sim != atools::fs::FsPaths::XPLANE11 && sim != atools::fs::FsPaths::NAVIGRAPH)
  {
    // Create VORTACs
    if((aborted = runScript(&progress, "fs/db/update_vor.sql", tr("Merging VOR and TACAN to VORTAC"))))
      return;
  }

  // Set the nav_ids (VOR, NDB) in the waypoint table and update the airway counts
  if((aborted = runScript(&progress, "fs/db/update_wp_ids.sql", tr("Updating waypoints"))))
    return;

  if(sim == atools::fs::FsPaths::NAVIGRAPH)
  {
    // Remove all unreferenced dummy waypoints that were added for airway generation
    if((aborted = runScript(&progress, "fs/db/dfd/clean_waypoints.sql", tr("Merging VOR and TACAN to VORTAC"))))
      return;
  }

  // Set the runway_end_ids in the approach table
  if((aborted = runScript(&progress, "fs/db/update_approaches.sql", tr("Updating approaches"))))
    return;

  if((aborted = runScript(&progress, "fs/db/update_airport.sql", tr("Updating Airports"))))
    return;

  if(sim == atools::fs::FsPaths::DFD)
  {
    if((aborted = runScript(&progress, "fs/db/dfd/update_airport_ils.sql", tr("Updating ILS"))))
      return;
  }
  else if(sim != atools::fs::FsPaths::XPLANE11)
  {
    // The ids are already updated when reading the X-Plane data
    // Set runway end ids into the ILS
    if((aborted = runScript(&progress, "fs/db/update_airport_ils.sql", tr("Updating ILS"))))
      return;
  }

  // update the ILS count in the airport table
  if((aborted = runScript(&progress, "fs/db/update_num_ils.sql", tr("Updating ILS Count"))))
    return;

  // Prepare the search table
  if((aborted = runScript(&progress, "fs/db/populate_nav_search.sql", tr("Collecting navaids for search"))))
    return;

  if(options->isCreateRouteTables())
  {
    // Fill tables for automatic flight plan calculation
    if((aborted = runScript(&progress, "fs/db/populate_route_node.sql", tr("Populating routing tables"))))
      return;

    if((aborted = progress.reportOther(tr("Creating route edges for VOR and NDB"))))
      return;

    // Create a network of VOR and NDB stations that allow radio navaid routing
    atools::fs::db::RouteEdgeWriter edgeWriter(db);
    edgeWriter.run();

    if((aborted = runScript(&progress, "fs/db/populate_route_edge.sql", tr("Creating route edges waypoints"))))
      return;
  }

  if((aborted = runScript(&progress, "fs/db/finish_airport_schema.sql", tr("Creating indexes for airport"))))
    return;

  if(sim != atools::fs::FsPaths::XPLANE11 && sim != atools::fs::FsPaths::NAVIGRAPH)
  {
    if((aborted = runScript(&progress, "fs/db/update_sea_base.sql", tr("Clean up runways"))))
      return;
  }

  if((aborted = runScript(&progress, "fs/db/finish_schema.sql", tr("Creating indexes for search"))))
    return;

  if(options->isCreateRouteTables())
  {
    if((aborted = runScript(&progress, "fs/db/finish_schema_route.sql", tr("Creating indexes for route"))))
      return;
  }

  // =====================================================================
  // Update the metadata in the database
  atools::fs::db::DatabaseMeta databaseMetadata(db);

  if(!xpDataCompiler.isNull())
    databaseMetadata.setAiracCycle(xpDataCompiler->getAiracCycle());
  if(!dfdCompiler.isNull())
    databaseMetadata.setAiracCycle(dfdCompiler->getAiracCycle(), dfdCompiler->getValidThrough());

  databaseMetadata.setDataSource(FsPaths::typeToShortName(sim));
  databaseMetadata.setCompilerVersion(QString("atools %1 (revision %2) %3 %4 (%5)").
                                      arg(atools::version()).
                                      arg(atools::gitRevision()).
                                      arg(QApplication::applicationName()).
                                      arg(QApplication::applicationVersion()).
                                      arg(gitRevision));

  databaseMetadata.updateAll();
  db->commit();

  if(!dfdCompiler.isNull())
    // database is kept locked by queries - need to close this late to avoid statistics generation for attached
    dfdCompiler->detachDatabase();

  // ================================================================================================
  // Done here - now only some options statistics and reports are left

  if(options->isBasicValidation())
    basicValidation(&progress);

  if(options->isDatabaseReport())
  {
    // Do a report of problems rather than failing totally during loading
    if(!fsDataWriter.isNull())
      fsDataWriter->logResults();
    createDatabaseReport(&progress);
  }

  if(options->isDropIndexes())
  {
    if((aborted = progress.reportOther(tr("Creating Database preparation Script"))))
      return;

    createPreparationScript();

    if((aborted = progress.reportOther(tr("Dropping All Indexes"))))
      return;

    dropAllIndexes();
  }
  if(options->isVacuumDatabase())
  {
    if((aborted = progress.reportOtherInc(tr("Vacuum Database"), PROGRESS_NUM_TASK_STEPS)))
      return;

    db->vacuum();
  }

  if(options->isAnalyzeDatabase())
  {
    if((aborted = progress.reportOtherInc(tr("Analyze Database"), PROGRESS_NUM_TASK_STEPS)))
      return;

    db->analyze();
  }

  // Send the final progress report
  progress.reportFinish();

  qDebug() << "Time" << timer.elapsed() / 1000 << "seconds";
}

bool NavDatabase::loadDfd(ProgressHandler *progress, ng::DfdCompiler *dfdCompiler, const scenery::SceneryArea& area)
{
  progress->reportSceneryArea(&area);

  dfdCompiler->writeFileAndSceneryMetadata();

  dfdCompiler->attachDatabase();

  dfdCompiler->initQueries();
  dfdCompiler->compileMagDeclBgl();
  dfdCompiler->readHeader();
  dfdCompiler->writeMora();

  if(options->isIncludedNavDbObject(atools::fs::type::AIRPORT))
  {
    dfdCompiler->writeAirports();
    if(options->isIncludedNavDbObject(atools::fs::type::RUNWAY))
      dfdCompiler->writeRunways();
  }
  if(options->isIncludedNavDbObject(atools::fs::type::WAYPOINT) ||
     options->isIncludedNavDbObject(atools::fs::type::VOR) ||
     options->isIncludedNavDbObject(atools::fs::type::NDB) ||
     options->isIncludedNavDbObject(atools::fs::type::MARKER) ||
     options->isIncludedNavDbObject(atools::fs::type::ILS))
    dfdCompiler->writeNavaids();

  if(options->isIncludedNavDbObject(atools::fs::type::BOUNDARY))
  {
    dfdCompiler->writeAirspaces();
    dfdCompiler->writeAirspaceCom();
  }

  dfdCompiler->writeCom();

  if((aborted = runScript(progress, "fs/db/create_indexes_post_load.sql", tr("Creating indexes"))))
    return true;

  if((aborted = runScript(progress, "fs/db/create_indexes_post_load_boundary.sql", tr("Creating boundary indexes"))))
    return true;

  if(options->isDeduplicate())
  {
    // Delete duplicates before any foreign keys ids are assigned
    if((aborted = runScript(progress, "fs/db/delete_duplicates.sql", tr("Clean up"))))
      return true;
  }

  if(options->isIncludedNavDbObject(atools::fs::type::AIRWAY))
    dfdCompiler->writeAirways();

  // Create waypoints for fix resolution in procedures - has to be done after airway processing
  if((aborted = runScript(progress, "fs/db/dfd/populate_navaids_proc.sql", tr("Creating waypoints for procedures"))))
    return true;

  dfdCompiler->updateMagvar();
  dfdCompiler->updateTacanChannel();
  dfdCompiler->updateIlsGeometry();

  if(options->isIncludedNavDbObject(atools::fs::type::APPROACH))
    dfdCompiler->writeProcedures();
  db->commit();

  if((aborted = runScript(progress, "fs/db/create_indexes_post_load.sql", tr("Creating indexes"))))
    return true;

  if((aborted = runScript(progress, "fs/db/create_indexes_post_load_boundary.sql", tr("Creating boundary indexes"))))
    return true;

  db->commit();

  // Update airport_id from ndb, vor and waypoint
  if((aborted = runScript(progress, "fs/db/dfd/update_navaids.sql", tr("Updating Navids in Waypoint"))))
    return true;

  db->commit();

  dfdCompiler->updateTreeLetterAirportCodes();

  db->commit();

  // dfdCompiler->removeDummyWaypoints();

  dfdCompiler->deInitQueries();

  return false;
}

bool NavDatabase::loadXplane(ProgressHandler *progress, atools::fs::xp::XpDataCompiler *xpDataCompiler,
                             const atools::fs::scenery::SceneryArea& area)
{
  if((aborted = progress->reportSceneryArea(&area)))
    return true;

  if((aborted = xpDataCompiler->writeBasepathScenery()))
    return true;

  if((aborted = xpDataCompiler->compileMagDeclBgl()))
    return true;

  if(options->isIncludedNavDbObject(atools::fs::type::AIRPORT))
  {
    // Airports are overloaded by ident

    // X-Plane 11/Custom Scenery/KSEA Demo Area/Earth nav data/apt.dat
    if((aborted = xpDataCompiler->compileCustomApt())) // Add-on
      return true;

    // X-Plane 11/Custom Scenery/Global Airports/Earth nav data/apt.dat
    if((aborted = xpDataCompiler->compileCustomGlobalApt()))
      return true;

    // X-Plane 11/Resources/default scenery/default apt dat/Earth nav data/apt.dat
    // Mandatory
    if((aborted = xpDataCompiler->compileDefaultApt()))
      return true;

    if((aborted = xpDataCompiler->fixDuplicateApt()))
      return true;
  }

  if(options->isIncludedNavDbObject(atools::fs::type::ILS))
  {
    // ILS corrections - "X-PLane/Custom Scenery/Global Airports/Earth nav data/earth_nav.dat"
    if((aborted = xpDataCompiler->compileLocalizers()))
      return true;
  }

  if(options->isIncludedNavDbObject(atools::fs::type::WAYPOINT))
  {
    // In resources or Custom Data - mandatory
    if((aborted = xpDataCompiler->compileEarthFix()))
      return true;

    // Optional user data
    if((aborted = xpDataCompiler->compileUserFix()))
      return true;
  }

  if(options->isIncludedNavDbObject(atools::fs::type::VOR) ||
     options->isIncludedNavDbObject(atools::fs::type::NDB) ||
     options->isIncludedNavDbObject(atools::fs::type::MARKER) ||
     options->isIncludedNavDbObject(atools::fs::type::ILS))
  {
    // In resources or Custom Data - mandatory
    if((aborted = xpDataCompiler->compileEarthNav()))
      return true;

    // Optional user data
    if((aborted = xpDataCompiler->compileUserNav()))
      return true;
  }

  if((aborted = runScript(progress, "fs/db/create_indexes_post_load.sql", tr("Creating indexes"))))
    return true;

  if((aborted = runScript(progress, "fs/db/create_indexes_post_load_boundary.sql", tr("Creating boundary indexes"))))
    return true;

  if(options->isIncludedNavDbObject(atools::fs::type::BOUNDARY))
  {
    // Airspaces
    if((aborted = xpDataCompiler->compileAirspaces()))
      return true;
  }

  if(options->isDeduplicate())
  {
    // Delete duplicates before any foreign keys ids are assigned
    if((aborted = runScript(progress, "fs/db/delete_duplicates.sql", tr("Clean up"))))
      return true;
  }

  if(options->isIncludedNavDbObject(atools::fs::type::AIRWAY))
  {
    // In resources or Custom Data - mandatory
    if((aborted = xpDataCompiler->compileEarthAirway()))
      return true;

    if((aborted = runScript(progress, "fs/db/xplane/prepare_airway.sql", tr("Preparing Airways"))))
      return true;

    if((aborted = xpDataCompiler->postProcessEarthAirway()))
      return true;
  }

  if(options->isIncludedNavDbObject(atools::fs::type::APPROACH))
  {
    if((aborted = xpDataCompiler->compileCifp()))
      return true;
  }
  db->commit();
  return false;
}

bool NavDatabase::loadFsxP3d(ProgressHandler *progress, atools::fs::db::DataWriter *fsDataWriter, const SceneryCfg& cfg)
{
  // Prepare structure for error collection
  NavDatabaseErrors::SceneryErrors err;
  fsDataWriter->setSceneryErrors(errors != nullptr ? &err : nullptr);
  fsDataWriter->readMagDeclBgl();
  if((!err.fileErrors.isEmpty() || !err.sceneryErrorsMessages.isEmpty()) && errors != nullptr)
    errors->sceneryErrors.append(err);

  qInfo() << Q_FUNC_INFO << "Scenery configuration ================================================";
  qInfo() << cfg;

  for(const atools::fs::scenery::SceneryArea& area : cfg.getAreas())
  {
    if((area.isActive() || options->isReadInactive()) &&
       options->isIncludedLocalPath(area.getLocalPath()))
    {
      if((aborted = progress->reportSceneryArea(&area)))
        return true;

      err = NavDatabaseErrors::SceneryErrors();
      fsDataWriter->setSceneryErrors(errors != nullptr ? &err : nullptr);

      // Read all BGL files in the scenery area into classes of the bgl namespace and
      // write the contents to the database
      fsDataWriter->writeSceneryArea(area);

      if((!err.fileErrors.isEmpty() || !err.sceneryErrorsMessages.isEmpty()) && errors != nullptr)
      {
        err.scenery = area;
        errors->sceneryErrors.append(err);
      }

      if((aborted = fsDataWriter->isAborted()))
        return true;
    }
  }
  db->commit();

  if((aborted = runScript(progress, "fs/db/create_indexes_post_load.sql", tr("Creating indexes"))))
    return true;

  if((aborted = runScript(progress, "fs/db/create_indexes_post_load_boundary.sql", tr("Creating boundary indexes"))))
    return true;

  if(options->isDeduplicate())
  {
    // Delete duplicates before any foreign keys ids are assigned
    if((aborted = runScript(progress, "fs/db/delete_duplicates.sql", tr("Clean up"))))
      return true;
  }

  return false;
}

bool NavDatabase::basicValidation(ProgressHandler *progress)
{
  if((aborted = progress->reportOther(tr("Basic Validation"))))
    return true;

  for(const QString& table : options->getBasicValidationTables().keys())
    basicValidateTable(table, options->getBasicValidationTables().value(table));

  return false;
}

void NavDatabase::basicValidateTable(const QString& table, int minCount)
{
  SqlUtil util(db);
  if(!util.hasTable(table))
    throw Exception("Table \"" + table + "\" not found.");

  int count = 0;
  if((count = util.rowCount(table)) < minCount)
    throw Exception(QString("Table \"%1\" has only %2 rows. Minimum required is %3").arg(table).arg(count).arg(minCount));

  qInfo() << "Table" << table << "is OK. Has" << count << "rows. Minimum required is" << minCount;
}

void NavDatabase::runPreparationPost245(atools::sql::SqlDatabase& db)
{
  qDebug() << Q_FUNC_INFO;

  SqlUtil util(db);

  // Remove the unneeded routing tables since data is loaded dynamically in newer versions
  if(util.hasTable("route_edge_airway"))
    db.exec("delete from route_edge_airway");
  if(util.hasTable("route_edge_radio"))
    db.exec("delete from route_edge_radio");
  if(util.hasTable("route_node_airway"))
    db.exec("delete from route_node_airway");
  if(util.hasTable("route_node_radio"))
    db.exec("delete from route_node_radio");
  db.commit();

  // Remove artificial waypoints since procedures now use coordinates and all navaids to resolve fixes
  if(util.hasTableAndColumn("waypoint", "artificial"))
    db.exec("delete from waypoint where artificial = 2");
  db.commit();

  // Delete legacy center boundaries in favor of new types FIR and UIR
  db.exec("delete from boundary where type = 'C' and name in ('% (FIR)', '% (UIR)', '% (FIR/UIR)')");
  db.commit();
}

void NavDatabase::runPreparationScript(atools::sql::SqlDatabase& db)
{
  qDebug() << Q_FUNC_INFO;
  if(SqlUtil(db).hasTableAndRows("script"))
  {
    SqlQuery scriptQuery("select statement from script ", db);
    scriptQuery.exec();
    while(scriptQuery.next())
    {
      qDebug() << "prepare script" << scriptQuery.valueStr("statement");
      SqlQuery query = db.exec(scriptQuery.valueStr("statement"));
      qDebug().nospace() << "[" << query.numRowsAffected() << "]";
    }
    db.commit();

    db.exec("delete from script");
    db.commit();
  }
}

void NavDatabase::createPreparationScript()
{
  if(SqlUtil(db).hasTable("script"))
  {
    SqlQuery insertScript(db);
    insertScript.prepare("insert into script (statement) values(:stmt)");

    SqlQuery indexQuery("select sql from sqlite_master where type = 'index' and sql is not null", db);
    indexQuery.exec();
    while(indexQuery.next())
    {
      insertScript.bindValue(":stmt", indexQuery.valueStr("sql"));
      insertScript.exec();
    }
  }
  db->commit();
}

void NavDatabase::dropAllIndexes()
{
  QStringList stmts;

  {
    SqlQuery indexQuery("select name from sqlite_master where type = 'index' and sql is not null", db);
    indexQuery.exec();
    while(indexQuery.next())
      stmts.append("drop index if exists " + indexQuery.valueStr("name"));
  }

  for(const QString& stmt : stmts)
    db->exec(stmt);
  db->commit();
}

bool NavDatabase::createDatabaseReport(ProgressHandler *progress)
{
  QDebug info(qInfo());
  atools::sql::SqlUtil util(db);

  if((aborted = progress->reportOther(tr("Creating table statistics"))))
    return true;

  info << endl;
  util.printTableStats(info);

  if((aborted = progress->reportOther(tr("Creating report on values"))))
    return true;

  info << endl;
  util.createColumnReport(info);

  if((aborted = progress->reportOther(tr("Creating report on duplicates"))))
    return true;

  info << endl;

  util.reportDuplicates(info, "airport", "airport_id", {"ident"});
  info << endl;

  util.reportDuplicates(info, "vor", "vor_id", {"ident", "region", "lonx", "laty"});
  info << endl;

  util.reportDuplicates(info, "ndb", "ndb_id", {"ident", "type", "frequency", "region", "lonx", "laty"});
  info << endl;

  util.reportDuplicates(info, "waypoint", "waypoint_id", {"ident", "type", "region", "lonx", "laty"});
  info << endl;

  util.reportDuplicates(info, "ils", "ils_id", {"ident", "lonx", "laty"});
  info << endl;

  util.reportDuplicates(info, "marker", "marker_id", {"type", "heading", "lonx", "laty"});
  info << endl;

  util.reportDuplicates(info, "helipad", "helipad_id", {"lonx", "laty"});
  info << endl;

  util.reportDuplicates(info, "parking", "parking_id", {"lonx", "laty"});
  info << endl;

  util.reportDuplicates(info, "start", "start_id", {"lonx", "laty"});
  info << endl;

  util.reportDuplicates(info, "runway", "runway_id", {"heading", "lonx", "laty"});
  info << endl;

  util.reportDuplicates(info, "bgl_file", "bgl_file_id", {"filename"});
  info << endl;

  if((aborted = progress->reportOther(tr("Creating report on coordinate duplicates"))))
    return true;

  reportCoordinateViolations(info, util, {"airport", "vor", "ndb", "marker", "waypoint"});

  return false;
}

bool NavDatabase::runScript(ProgressHandler *progress, const QString& scriptFile, const QString& message)
{
  SqlScript script(db, true /*options->isVerbose()*/);

  if(progress != nullptr)
    if((aborted = progress->reportOtherInc(message, PROGRESS_NUM_SCRIPT_STEPS)))
      return true;

  script.executeScript(":/atools/resources/sql/" + scriptFile);
  db->commit();
  return false;
}

void NavDatabase::readSceneryConfig(atools::fs::scenery::SceneryCfg& cfg)
{
  // Get entries from scenery.cfg file
  cfg.read(options->getSceneryFile());

  bool readInactive = options->isReadInactive();
  FsPaths::SimulatorType sim = options->getSimulatorType();

  if(options->isReadAddOnXml() &&
     (sim == atools::fs::FsPaths::P3D_V3 || sim == atools::fs::FsPaths::P3D_V4 || sim == atools::fs::FsPaths::P3D_V5))
  {
    // Read the Prepar3D add on packages and add them to the scenery list ===============================
    QString documents(atools::documentsDir());

    int simNum = 0;
    if(sim == atools::fs::FsPaths::P3D_V3)
      simNum = 3;
    else if(sim == atools::fs::FsPaths::P3D_V4)
      simNum = 4;
    else if(sim == atools::fs::FsPaths::P3D_V5)
      simNum = 5;

    // Calculate maximum area number
    int areaNum = std::numeric_limits<int>::min();
    for(const SceneryArea& area : cfg.getAreas())
      areaNum = std::max(areaNum, area.getAreaNumber());

    QStringList addonsCfgFiles;

    // The priority for how content based add-on configuration files are initialized is as follows:
    // Local: Configuration files found at: %LOCALAPPDATA%\Lockheed Martin\Prepar3D v4
    // Roaming: Configuration files found at: %APPDATA%\Lockheed Martin\Prepar3D v4
    // ProgramData: Configuration files found at: %PROGRAMDATA%\Lockheed Martin\Prepar3D v4

    // Read add-ons.cfg file from local =========================
    {
#if defined(Q_OS_WIN32)
      // Use the environment variable because QStandardPaths::ConfigLocation returns an unusable path on Windows
      QString addonsCfgFileLocal = QProcessEnvironment::systemEnvironment().value("LOCALAPPDATA");
#else
      // Use $HOME/.config for testing
      QString addonsCfgFileLocal = QStandardPaths::standardLocations(QStandardPaths::ConfigLocation).first();
#endif
      addonsCfgFileLocal += SEP + QString("Lockheed Martin") + SEP + QString("Prepar3D v%1").arg(simNum) +
#if !defined(Q_OS_WIN32)
                            " LocalData" +
#endif
                            SEP + "add-ons.cfg";
      addonsCfgFiles.append(addonsCfgFileLocal);
    }

    // Read add-ons.cfg file from roaming =========================
    {

#if defined(Q_OS_WIN32)
      // Use the environment variable because QStandardPaths::ConfigLocation returns an unusable path on Windows
      QString addonsCfgFile = QProcessEnvironment::systemEnvironment().value("APPDATA");
#else
      // Use $HOME/.config for testing
      QString addonsCfgFile = QStandardPaths::standardLocations(QStandardPaths::ConfigLocation).first();
#endif
      addonsCfgFile +=
        SEP + QString("Lockheed Martin") +
        SEP + QString("Prepar3D v%1").arg(simNum) +
        SEP + "add-ons.cfg";
      addonsCfgFiles.append(addonsCfgFile);
    }

    // Read the add-ons.cfg from ProgramData =========================
    // "C:\\ProgramData\\Lockheed Martin\\Prepar3D v3\\add-ons.cfg"
    {
#if defined(Q_OS_WIN32)
      // Use the environment variable because QStandardPaths::ConfigLocation returns an unusable path on Windows
      QString addonsAllUsersCfgFile = QProcessEnvironment::systemEnvironment().value("PROGRAMDATA");
#else
      // Use /tmp for testing
      QString addonsAllUsersCfgFile = QStandardPaths::standardLocations(QStandardPaths::ConfigLocation).first();
#endif
      addonsAllUsersCfgFile +=
        SEP + QString("Lockheed Martin") +
        SEP + QString("Prepar3D v%1").arg(simNum) +
#if !defined(Q_OS_WIN32)
        " ProgramData" +
#endif
        SEP + "add-ons.cfg";
      addonsCfgFiles.append(addonsAllUsersCfgFile);
    }

    // ==================================================================
    // Read all add-ons.cfg files from the paths
    // Use this to weed out duplicates to the add-on.xml files
    QSet<QString> addonFilePaths;
    // Set layer later to these
    QVector<AddOnComponent> noLayerComponents;
    QStringList noLayerPaths;
    QStringList addonDiscoveryPaths;
    QSet<QString> inactiveAddOnPaths;

    for(const QString& addonsCfg : addonsCfgFiles)
    {
      if(QFileInfo::exists(addonsCfg))
      {
        qInfo() << Q_FUNC_INFO << "Reading" << addonsCfg;
        AddOnCfg addonConfigProgramData("utf-8");
        addonConfigProgramData.read(addonsCfg);

        for(const AddOnCfgEntry& entry:addonConfigProgramData.getEntriesDiscovery())
        {
          if(entry.active || readInactive)
            addonDiscoveryPaths.append(QFileInfo(entry.path).canonicalFilePath());
        }

        for(const AddOnCfgEntry& entry:addonConfigProgramData.getEntries())
        {
          if(entry.active || readInactive)
            readAddOnComponents(areaNum, cfg, noLayerComponents, noLayerPaths, addonFilePaths, QFileInfo(entry.path));
          else
            inactiveAddOnPaths.insert(buildAddonFile(QFileInfo(entry.path)).canonicalFilePath().toLower());
        }
      }
    }

    // Go through the two or more discovery paths ===============
    // Add both path alternatives since documentation is not clear
    // Mentioned in the SDK on "Add-on Packages" -> "Distributing an Add-on Package"
    // Mentioned in the SDK on "Add-on Instructions for Developers" -> "Add-on Directory Structure"
    addonDiscoveryPaths.prepend(documents + SEP + QString("Prepar3D v%1 Files").arg(simNum) +
                                SEP + QLatin1Literal("add-ons"));

    addonDiscoveryPaths.prepend(documents + SEP + QString("Prepar3D v%1 Add-ons").arg(simNum));

    qInfo() << Q_FUNC_INFO << "Discovery paths" << addonDiscoveryPaths;

    // ====================================================================================
    // Read add-on.xml files from the discovery paths
    for(const QString& addonPath : addonDiscoveryPaths)
    {
      QDir addonDir(addonPath);
      if(addonDir.exists())
      {
        QFileInfoList addonEntries(addonDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot));

        // Read addon directories as they appear in the file system
        for(QFileInfo addonEntry : addonEntries)
        {
          if(readInactive || !inactiveAddOnPaths.contains(buildAddonFile(addonEntry).canonicalFilePath().toLower()))
            readAddOnComponents(areaNum, cfg, noLayerComponents, noLayerPaths, addonFilePaths, addonEntry);
          else
            qInfo() << Q_FUNC_INFO << "Skipping inactive" << addonEntry.canonicalFilePath();
        }
      }
      else
        qWarning() << Q_FUNC_INFO << addonDir << "does not exist";
    }

    // Bring added add-on.xml in order with the rest sort by layer
    cfg.sortAreas();

    // Calculate maximum layer and area number
    int lastLayer = std::numeric_limits<int>::min();
    int lastArea = std::numeric_limits<int>::min();
    for(const SceneryArea& area : cfg.getAreas())
    {
      lastArea = std::max(lastArea, area.getAreaNumber());
      lastLayer = std::max(lastLayer, area.getLayer());
    }

    for(int i = 0; i < noLayerComponents.size(); i++)
      cfg.appendArea(SceneryArea(++lastArea, ++lastLayer, noLayerComponents.at(i).getName(), noLayerPaths.at(i)));
  } // if(options->isReadAddOnXml()

  // Check if some areas have to be sorted to the end of the list
  for(SceneryArea& area : cfg.getAreas())
  {
    if(options->isHighPriority(area.getLocalPath()))
    {
      area.setHighPriority();
      qInfo() << Q_FUNC_INFO << "Moving to highest layer:" << area;
    }
  }

  // Sort again to get high priority layers to the end of the list
  cfg.sortAreas();
}

QFileInfo NavDatabase::buildAddonFile(const QFileInfo& addonEntry)
{
  return QFileInfo(addonEntry.canonicalFilePath() + SEP + QLatin1Literal("add-on.xml"));
}

void NavDatabase::readAddOnComponents(int& areaNum, atools::fs::scenery::SceneryCfg& cfg,
                                      QVector<AddOnComponent>& noLayerComponents, QStringList& noLayerPaths,
                                      QSet<QString>& addonPaths, const QFileInfo& addonEntry)
{
  QFileInfo addonFile = buildAddonFile(addonEntry);

  if(addonFile.exists() && addonFile.isFile())
  {
    if(addonPaths.contains(addonFile.canonicalFilePath()))
    {
      qInfo() << "Found duplicate addon file" << addonFile.filePath();
      return;
    }

    qInfo() << "Found addon file" << addonFile.filePath();
    addonPaths.insert(addonFile.canonicalFilePath());

    AddOnPackage package(addonFile.filePath());
    qInfo() << "Name" << package.getName() << "Description" << package.getDescription();

    for(const AddOnComponent& component : package.getComponents())
    {
      qInfo() << "Component" << component.getLayer()
              << "Name" << component.getName()
              << "Path" << component.getPath();

      QDir compPath(component.getPath());

      if(compPath.isRelative())
        // Convert relative path to absolute based on add-on file directory
        compPath = package.getBaseDirectory() + SEP + compPath.path();

      if(compPath.dirName().toLower() == "scenery")
        // Remove if it points to scenery directory
        compPath.cdUp();

      compPath.makeAbsolute();

      areaNum++;

      if(!compPath.exists())
        qWarning() << "Path does not exist" << compPath;

      if(component.getLayer() == -1)
      {
        // Add entries without layers later at the end of the list
        // Layer is only used if add-on does not provide a layer
        noLayerComponents.append(component);
        noLayerPaths.append(compPath.path());
      }
      else
        cfg.appendArea(SceneryArea(areaNum, component.getLayer(), component.getName(), compPath.path()));
    }
  }
  else
    qWarning() << Q_FUNC_INFO << addonFile.filePath() << "does not exist or is not a directory";
}

void NavDatabase::reportCoordinateViolations(QDebug& out, atools::sql::SqlUtil& util,
                                             const QStringList& tables)
{
  for(QString table : tables)
  {
    out << "==================================================================" << endl;
    util.reportRangeViolations(out, table, {table + "_id", "ident"}, "lonx", -180.f, 180.f);
    util.reportRangeViolations(out, table, {table + "_id", "ident"}, "laty", -90.f, 90.f);
  }
}

void NavDatabase::countFiles(const atools::fs::scenery::SceneryCfg& cfg, int *numFiles, int *numSceneryAreas)
{
  qDebug() << "Counting files";

  for(const atools::fs::scenery::SceneryArea& area : cfg.getAreas())
  {
    if(area.isActive() && options->isIncludedLocalPath(area.getLocalPath()))
    {
      atools::fs::scenery::FileResolver resolver(*options, true);

      *numFiles += resolver.getFiles(area);
      (*numSceneryAreas)++;
    }
  }
  qDebug() << "Counting files done." << *numFiles << "files to process";
}

} // namespace fs
} // namespace atools
