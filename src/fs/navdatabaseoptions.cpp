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

#include "fs/navdatabaseoptions.h"
#include "scenery/sceneryarea.h"

#include <QDebug>
#include <QFileInfo>
#include <QList>
#include <QRegExp>
#include <QDir>
#include <QSettings>

namespace atools {
namespace fs {

NavDatabaseOptions::NavDatabaseOptions()
{
}

void NavDatabaseOptions::setProgressCallback(ProgressCallbackType func)
{
  progressCallback = func;
}

NavDatabaseOptions::ProgressCallbackType NavDatabaseOptions::getProgressCallback() const
{
  return progressCallback;
}

void NavDatabaseOptions::addToDirectoryExcludes(const QStringList& filter)
{
  addToFilter(createFilterList(filter), dirExcludesGui);
}

void NavDatabaseOptions::addToFilePathExcludes(const QStringList& filter)
{
  addToFilter(fromNativeSeparators(filter), filePathExcludesGui);
}

void NavDatabaseOptions::addToAddonDirectoryExcludes(const QStringList& filter)
{
  addToFilter(createFilterList(filter), addonDirExcludes);
}

bool NavDatabaseOptions::isIncludedLocalPath(const QString& filepath) const
{
  return includeObject(adaptPath(filepath), pathFiltersInc, pathFiltersExcl);
}

bool NavDatabaseOptions::isAddonLocalPath(const QString& filepath) const
{
  return includeObject(adaptPath(filepath), addonFiltersInc, addonFiltersExcl);
}

bool NavDatabaseOptions::isIncludedDirectory(const QString& dirpath) const
{
  return includeObject(adaptPath(dirpath), QList<QRegExp>(), dirExcludesGui);
}

bool NavDatabaseOptions::isIncludedFilePath(const QString& filepath) const
{
  return includeObject(fromNativeSeparator(filepath), QList<QRegExp>(), filePathExcludesGui);
}

bool NavDatabaseOptions::isHighPriority(const QString& filepath) const
{
  if(highPriorityFiltersInc.isEmpty())
    return false;

  return includeObject(adaptPath(filepath), highPriorityFiltersInc, QList<QRegExp>());
}

bool NavDatabaseOptions::isAddonDirectory(const QString& filepath) const
{
  return includeObject(adaptPath(filepath), QList<QRegExp>(), addonDirExcludes);
}

bool NavDatabaseOptions::isIncludedFilename(const QString& filename) const
{
  QString fn = QFileInfo(filename).fileName();
  return includeObject(fn, fileFiltersInc, fileFiltersExcl);
}

bool NavDatabaseOptions::isIncludedAirportIdent(const QString& icao) const
{
  return includeObject(icao, airportIcaoFiltersInc, airportIcaoFiltersExcl);
}

void NavDatabaseOptions::addToFilenameFilterInclude(const QStringList& filter)
{
  addToFilter(filter, fileFiltersInc);
}

void NavDatabaseOptions::addToAirportIcaoFilterInclude(const QStringList& filter)
{
  addToFilter(filter, airportIcaoFiltersInc);
}

void NavDatabaseOptions::addToPathFilterInclude(const QStringList& filter)
{
  addToFilter(fromNativeSeparators(filter), pathFiltersInc);
}

void NavDatabaseOptions::addToAddonFilterInclude(const QStringList& filter)
{
  addToFilter(fromNativeSeparators(filter), addonFiltersInc);
}

void NavDatabaseOptions::addToFilenameFilterExclude(const QStringList& filter)
{
  addToFilter(filter, fileFiltersExcl);
}

void NavDatabaseOptions::addToAirportIcaoFilterExclude(const QStringList& filter)
{
  addToFilter(filter, airportIcaoFiltersExcl);
}

void NavDatabaseOptions::addToPathFilterExclude(const QStringList& filter)
{
  addToFilter(fromNativeSeparators(filter), pathFiltersExcl);
}

void NavDatabaseOptions::addToAddonFilterExclude(const QStringList& filter)
{
  addToFilter(fromNativeSeparators(filter), addonFiltersExcl);
}

void NavDatabaseOptions::addToBglObjectFilterInclude(const QStringList& filters)
{
  addToBglObjectFilter(filters, navDbObjectTypeFiltersInc);
}

void NavDatabaseOptions::addToBglObjectFilterExclude(const QStringList& filters)
{
  addToBglObjectFilter(filters, navDbObjectTypeFiltersExcl);
}

void NavDatabaseOptions::addToHighPriorityFiltersInc(const QStringList& filters)
{
  addToFilter(filters, highPriorityFiltersInc);
}

void NavDatabaseOptions::addToBglObjectFilter(const QStringList& filters,
                                              QSet<atools::fs::type::NavDbObjectType>& filterList)
{
  for(const QString& f : filters)
    if(!f.isEmpty())
      filterList.insert(type::stringToNavDbObjectType(f));
}

bool NavDatabaseOptions::isIncludedNavDbObject(type::NavDbObjectType type) const
{
  if(navDbObjectTypeFiltersInc.isEmpty() && navDbObjectTypeFiltersExcl.isEmpty())
    return true;

  bool exFound = navDbObjectTypeFiltersExcl.contains(type);

  if(navDbObjectTypeFiltersInc.isEmpty())
    return !exFound;
  else
  {
    bool incFound = navDbObjectTypeFiltersInc.contains(type);

    if(navDbObjectTypeFiltersExcl.isEmpty())
      return incFound;
    else
      return incFound && !exFound;
  }
}

QStringList NavDatabaseOptions::createFilterList(const QStringList& pathList)
{
  QStringList retval;

  for(const QString& path : pathList)
  {
    QDir dir(path);
    QString newPath = dir.absolutePath().trimmed().replace("\\", "/");
    if(!newPath.endsWith("/"))
      newPath.append("/");
    newPath.append("*");
    retval.append(newPath);
  }
  return retval;
}

void NavDatabaseOptions::loadFromSettings(QSettings& settings)
{
  setReadInactive(settings.value("Options/IgnoreInactive", false).toBool());
  setVerbose(settings.value("Options/Verbose", false).toBool());
  setResolveAirways(settings.value("Options/ResolveAirways", true).toBool());
  setCreateRouteTables(settings.value("Options/CreateRouteTables", false).toBool());
  setDatabaseReport(settings.value("Options/DatabaseReport", true).toBool());
  setDeletes(settings.value("Options/ProcessDelete", true).toBool());
  setDeduplicate(settings.value("Options/Deduplicate", true).toBool());
  setFilterOutDummyRunways(settings.value("Options/FilterRunways", true).toBool());
  setWriteIncompleteObjects(settings.value("Options/SaveIncomplete", true).toBool());
  setAutocommit(settings.value("Options/Autocommit", false).toBool());
  setFlag(type::BASIC_VALIDATION, settings.value("Options/BasicValidation", false).toBool());
  setFlag(type::VACUUM_DATABASE, settings.value("Options/VacuumDatabase", true).toBool());
  setFlag(type::ANALYZE_DATABASE, settings.value("Options/AnalyzeDatabase", true).toBool());
  setFlag(type::DROP_INDEXES, settings.value("Options/DropAllIndexes", false).toBool());

  addToHighPriorityFiltersInc(settings.value("Filter/IncludeHighPriorityFilter").toStringList());

  addToFilenameFilterInclude(settings.value("Filter/IncludeFilenames").toStringList());
  addToFilenameFilterExclude(settings.value("Filter/ExcludeFilenames").toStringList());
  addToPathFilterInclude(settings.value("Filter/IncludePathFilter").toStringList());
  addToPathFilterExclude(settings.value("Filter/ExcludePathFilter").toStringList());
  addToAddonFilterInclude(settings.value("Filter/IncludeAddonPathFilter").toStringList());
  addToAddonFilterExclude(settings.value("Filter/ExcludeAddonPathFilter").toStringList());
  addToAirportIcaoFilterInclude(settings.value("Filter/IncludeAirportIcaoFilter").toStringList());
  addToAirportIcaoFilterExclude(settings.value("Filter/ExcludeAirportIcaoFilter").toStringList());
  addToBglObjectFilterInclude(settings.value("Filter/IncludeBglObjectFilter").toStringList());
  addToBglObjectFilterExclude(settings.value("Filter/ExcludeBglObjectFilter").toStringList());

  settings.beginGroup("BasicValidationTables");

  QString simStr = simulatorType == FsPaths::DFD ? "DFD" : FsPaths::typeToShortName(simulatorType);

  for(const QString& key : settings.childKeys())
  {
    // Addd keys without prefix or keys with matching prefix
    if(!key.contains('.') || key.section('.', 0, 0) == simStr)
      basicValidationTables.insert(key.section('.', 1, 1), settings.value(key).toInt());
  }

  settings.endGroup();
}

bool NavDatabaseOptions::includeObject(const QString& string, const QList<QRegExp>& filterListInc,
                                       const QList<QRegExp>& filterListExcl) const
{
  if(filterListInc.isEmpty() && filterListExcl.isEmpty())
    return true;

  bool excludeMatched = false;
  for(const QRegExp& iter : filterListExcl)
  {
    if(iter.exactMatch(string))
    {
      excludeMatched = true;
      break;
    }
  }

  if(filterListInc.isEmpty())
    // No include filters - let exclude filter decide
    return !excludeMatched;
  else
  {
    bool includeMatched = false;
    for(const QRegExp& iter : filterListInc)
    {
      if(iter.exactMatch(string))
      {
        includeMatched = true;
        break;
      }
    }

    if(filterListExcl.isEmpty())
      // No exclude filters - let include filter decide
      return includeMatched;
    else
      return includeMatched && !excludeMatched;
  }
}

void NavDatabaseOptions::addToFilter(const QStringList& filters, QList<QRegExp>& filterList)
{
  for(QString f : filters)
  {
    QString newFilter = f.trimmed();
    if(!newFilter.isEmpty())
      filterList.append(QRegExp(newFilter, Qt::CaseInsensitive, QRegExp::Wildcard));
  }
}

QString NavDatabaseOptions::adaptPath(const QString& filepath) const
{
  // make sure that backslashes are replaced and path is suffixed with a slash
  QString newFilename = fromNativeSeparator(filepath);
  if(!filepath.endsWith("/"))
    newFilename.append("/");

  return newFilename;
}

QString NavDatabaseOptions::fromNativeSeparator(const QString& path) const
{
  // make sure that backslashes are replaced
  return QString(path).replace("\\", "/");
}

QStringList NavDatabaseOptions::fromNativeSeparators(const QStringList& paths) const
{
  QStringList retval;
  for(const QString& p : paths)
    retval.append(fromNativeSeparator(p));
  return retval;
}

QDebug operator<<(QDebug out, const NavDatabaseOptions& opts)
{
  QDebugStateSaver saver(out);
  out.nospace().noquote() << "Options[flags " << opts.flags;

  out << ", Include file filter [";
  for(const QRegExp& f : opts.fileFiltersInc)
    out << f.pattern() << ", ";
  out << "]";

  out << ", Exclude file filter [";
  for(const QRegExp& f : opts.fileFiltersExcl)
    out << f.pattern() << ", ";
  out << "]";

  out << ", Include path filter [";
  for(const QRegExp& f : opts.pathFiltersInc)
    out << f.pattern() << ", ";
  out << "]";

  out << ", Exclude path filter [";
  for(const QRegExp& f : opts.pathFiltersExcl)
    out << f.pattern() << ", ";
  out << "]";

  out << ", Include airport filter [";
  for(const QRegExp& f : opts.airportIcaoFiltersInc)
    out << f.pattern() << ", ";
  out << "]";

  out << ", Exclude airport filter [";
  for(const QRegExp& f : opts.airportIcaoFiltersExcl)
    out << f.pattern() << ", ";
  out << "]";

  out << ", Include addon filter [";
  for(const QRegExp& f : opts.addonFiltersInc)
    out << f.pattern() << ", ";
  out << "]";

  out << ", Exclude addon filter [";
  for(const QRegExp& f : opts.addonFiltersExcl)
    out << f.pattern() << ", ";
  out << "]";

  out << ", Include high priority filter [";
  for(const QRegExp& f : opts.highPriorityFiltersInc)
    out << f.pattern() << ", ";
  out << "]";

  out << ", Exclude directory filter [";
  for(const QRegExp& f : opts.dirExcludesGui)
    out << f.pattern() << ", ";
  out << "]";

  out << ", Exclude addon directory filter [";
  for(const QRegExp& f : opts.addonDirExcludes)
    out << f.pattern() << ", ";
  out << "]";

  out << ", Include type filter [";
  for(type::NavDbObjectType type : opts.navDbObjectTypeFiltersInc)
    out << type::navDbObjectTypeToString(type) << ", ";
  out << "]";
  out << ", Exclude type filter [";
  for(type::NavDbObjectType type : opts.navDbObjectTypeFiltersExcl)
    out << type::navDbObjectTypeToString(type) << ", ";

  out << "]";
  out << "]";
  return out;
}

QString type::navDbObjectTypeToString(type::NavDbObjectType type)
{
  switch(type)
  {
    case AIRPORT:
      return "AIRPORT";

    case RUNWAY:
      return "RUNWAY";

    case HELIPAD:
      return "HELIPAD";

    case START:
      return "START";

    case APPROACH:
      return "APPROACH";

    case APPROACHLEG:
      return "APPROACHLEG";

    case COM:
      return "COM";

    case PARKING:
      return "PARKING";

    case ILS:
      return "ILS";

    case VOR:
      return "VOR";

    case NDB:
      return "NDB";

    case WAYPOINT:
      return "WAYPOINT";

    case BOUNDARY:
      return "BOUNDARY";

    case MARKER:
      return "MARKER";

    case AIRWAY:
      return "AIRWAY";

    case APRON:
      return "APRON";

    case APRON2:
      return "APRON2";

    case APRONLIGHT:
      return "APRONLIGHT";

    case FENCE:
      return "FENCE";

    case TAXIWAY:
      return "TAXIWAY";

    case TAXIWAY_RUNWAY:
      return "TAXIWAY_RUNWAY";

    case VEHICLE:
      return "VEHICLE";

    case GEOMETRY:
      return "GEOMETRY";

    case UNKNOWN:
      return "UNKNWON";
  }
  return "UNKNWON";
}

type::NavDbObjectType type::stringToNavDbObjectType(const QString& typeStr)
{
  if(typeStr == "AIRPORT")
    return AIRPORT;
  else if(typeStr == "RUNWAY")
    return RUNWAY;
  else if(typeStr == "HELIPAD")
    return HELIPAD;
  else if(typeStr == "START")
    return START;
  else if(typeStr == "APPROACH")
    return APPROACH;
  else if(typeStr == "APPROACHLEG")
    return APPROACHLEG;
  else if(typeStr == "COM")
    return COM;
  else if(typeStr == "PARKING")
    return PARKING;
  else if(typeStr == "ILS")
    return ILS;
  else if(typeStr == "VOR")
    return VOR;
  else if(typeStr == "NDB")
    return NDB;
  else if(typeStr == "WAYPOINT")
    return WAYPOINT;
  else if(typeStr == "BOUNDARY")
    return BOUNDARY;
  else if(typeStr == "MARKER")
    return MARKER;
  else if(typeStr == "APRON")
    return APRON;
  else if(typeStr == "APRON2")
    return APRON2;
  else if(typeStr == "APRONLIGHT")
    return APRONLIGHT;
  else if(typeStr == "FENCE")
    return FENCE;
  else if(typeStr == "TAXIWAY")
    return TAXIWAY;
  else if(typeStr == "VEHICLE")
    return VEHICLE;
  else if(typeStr == "TAXIWAY_RUNWAY")
    return TAXIWAY_RUNWAY;
  else if(typeStr == "AIRWAY")
    return AIRWAY;
  else if(typeStr == "GEOMETRY")
    return GEOMETRY;
  else
    return UNKNOWN;
}

} // namespace fs
} // namespace atools
