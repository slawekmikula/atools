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

#ifndef ATOOLS_GRIB_WINDQUERY_H
#define ATOOLS_GRIB_WINDQUERY_H

#include "grib/gribcommon.h"

#include <QObject>

#include "geo/pos.h"
#include "atools.h"

class QObject;

namespace atools {
namespace util {
class FileSystemWatcher;
}

namespace geo {
class Rect;
class Line;
class LineString;
}

namespace grib {

class GribDownloader;

Q_DECL_CONSTEXPR static float INVALID_WIND_DIR_VALUE = std::numeric_limits<float>::max();
Q_DECL_CONSTEXPR static float INVALID_WIND_SPEED_VALUE = std::numeric_limits<float>::max();

/* Combines wind speed and direction */
struct Wind
{
  Wind(float windDir, float windSpeed)
  {
    dir = windDir;
    speed = windSpeed;
  }

  Wind()
  {
    dir = speed = 0.f;
  }

  /* Degrees true and knots */
  float dir, speed;

  bool isValid() const
  {
    return speed >= 0.f && speed < 1000.f && dir >= 0.f && dir <= 360.f;
  }

  bool isNull() const
  {
    return speed < 1.f;
  }

  bool operator==(const Wind& other) const
  {
    return atools::almostEqual(speed, other.speed) && atools::almostEqual(dir, other.dir);
  }

  bool operator!=(const Wind& other) const
  {
    return !operator==(other);
  }

};

/* Invalid wind */
const atools::grib::Wind EMPTY_WIND;

/* Combines wind speed and direction at a position */
struct WindPos
{
  atools::geo::Pos pos;
  Wind wind;

  bool isValid() const
  {
    return pos.isValid() && wind.isValid();
  }

  bool operator==(const WindPos& other) const
  {
    return pos == other.pos && wind == other.wind;
  }

  bool operator!=(const WindPos& other) const
  {
    return !operator==(other);
  }

};

/* Invalid wind pos */
const atools::grib::WindPos EMPTY_WIND_POS;

typedef QVector<WindPos> WindPosVector;
typedef QList<WindPos> WindPosList;

struct WindRect;
struct GridRect;
struct WindData;
struct WindAltLayer;

/*
 * Takes care for downloading/reading and decoding of GRIB2 wind files. Provides a query API to calculate and interpolate
 * winds for points, rectangles and lines.
 *
 * Wind speed and direction are interpolated for positions in the grid and given altitudes between layers.
 * Fetching data above the highest altitude will use the highest data.
 * Fetching data below the lowest altitude will interpolate between lowest layer and zero.
 *
 * Data is updated automatically every 30 minutes.
 * Files are checked for changes.
 *
 * All internal calculations the U and V components of the wind instead of speed and direction.
 * Most query methods use the altitude from the Pos parameter.
 *
 * Downloaded/possible sets are:
 * Altitide | Act. pressure | Pressure param | Downloaded | Used by X-Plane
 * 10000 ft | 69.7 mb       | 700            | *          | XP
 * 15000 ft | 57.2 mb       | 550            |            |
 * 20000 ft | 46.6 mb       | 450            | *          |
 * 25000 ft | 37.6 mb       | 350            |            |
 * 30000 ft | 30.1 mb       | 300            | *          |
 * 35000 ft | 23.8 mb       | 250            |            | XP
 * 40000 ft | 18.8 mb       | 200            | *          |
 * 45000 ft | 14.7 mb       | 150            | *          |
 * 50000 ft | 11.6 mb       | 100            |            |
 */
class WindQuery
  : public QObject
{
  Q_OBJECT

public:
  WindQuery(QObject *parentObject, bool logVerbose);
  ~WindQuery();

  /* Initialize download from https://nomads.ncep.noaa.gov/cgi-bin/filter_gfs_1p00.pl if baseUrl is empty
   *  Will download and update every 30 minutes and terminate any file watching. */
  void initFromUrl(const QString& baseUrl = QString());

  /* Read data from file and start watching for changes - terminates downloads */
  void initFromFile(const QString& filename);

  /* Read data from byte array */
  void initFromData(const QByteArray& data);

  /* Create a fixed model assuming zero wind at 0 altitude and given values at given altitude.
   *  Dir in degrees true, speed in knots and altutude in feet. */
  void initFromFixedModel(float dir, float speed, float altitude);

  /* Create a fixed model with two layers.
   *  Dir in degrees true, speed in knots and altutude in feet. */
  void initFromFixedModel(float dirLower, float speedLower, float altitudeLower, float dirUpper, float speedUpper,
                          float altitudeUpper);

  /* Clear data, stop periodic downloads and file watching */
  void deinit();

  /* Get interpolated wind data for single position. Altitude in feet is used from position. */
  Wind getWindForPos(const atools::geo::Pos& pos, bool interpolateValue = true) const;

  /* Get an array of wind data for the given rectangle at the given altitude from the data grid.
   * Data is only interpolated between layers. Result is sorted by y and x coordinates.*/
  void getWindForRect(atools::grib::WindPosVector& result, const geo::Rect& rect, float altFeet) const;
  atools::grib::WindPosVector getWindForRect(const atools::geo::Rect& rect, float altFeet) const;

  /* Get average wind for the great circle line between the two given positions at the given altitude.
   *  Wind data is fetched for a certain number of spots along the line and thus not perfectly accurate.
   * Uses altitude from positions and interpolates between altitudes too if they are different. */
  Wind getWindAverageForLine(const atools::geo::Pos& pos1, const atools::geo::Pos& pos2) const;
  Wind getWindAverageForLine(const atools::geo::Line& line) const;
  Wind getWindAverageForLineString(const atools::geo::LineString& linestring) const;

  bool hasWindData() const
  {
    return !windLayers.isEmpty();
  }

  /* Samples per degree for wind interpolation along lines and line strings */
  void setSamplesPerDegree(int value)
  {
    samplesPerDegree = value;
  }

  QString getDebug(const atools::geo::Pos& pos) const;

signals:
  /* Download successfully finished. Emitted for all init methods. */
  void windDataUpdated();

  /* Download failed.  Only for void init() and initFromFile(). */
  void windDownloadFailed(const QString& error, int errorCode);

private:
  /* Wind for grid position */
  WindData windForLayer(const WindAltLayer& layer, const QPoint& point) const;

  /* Get layer above and below (or at) altitude */
  void layersByAlt(WindAltLayer& lower, WindAltLayer& upper, float altitude) const;

  /* Fill cell rectangle with wind values at corners */
  void windRectForLayer(WindRect& windRect, const WindAltLayer& layer, const atools::grib::GridRect& rect) const;

  /* Quadratic interpolation- Returns wind for position in the grid cell */
  WindData interpolateRect(const WindRect& windRect, const geo::Rect& rect, const geo::Pos& pos) const;

  /* Convert data from U/V components to speed/heading */
  void convertDataset(const atools::grib::GribDatasetVector& datasets);

  void gribDownloadFinished(const atools::grib::GribDatasetVector & datasets, QString);
  void gribDownloadFailed(const QString & error, int errorCode, QString);
  void gribFileUpdated(const QString& filename);

  /* get interpolated wind for two sets at two altitudes */
  WindData interpolateWind(const WindData& w0, const WindData& w1, float alt0, float alt1, float alt) const;

  /* Get average wind for a line between two points. Uses only U and V components */
  WindData windAverageForLine(const atools::geo::Pos& pos1, const atools::geo::Pos& pos2) const;

  /* Surfaces to download from NOAA. Negative value denotes AGL in ft and positive is millibar */
  const QVector<int> SURFACES = {-80, 150, 200, 250, 300, 450, 700};
  /* Parameters to download from NOAA - U/V wind component */
  const QStringList PARAMETERS = {"UGRD", "VGRD"};

  /* Do roughly four samples per degree when interpolating winds for lines */
  int samplesPerDegree = 4;

  /* Used for regular downloads from NOAA site */
  atools::grib::GribDownloader *downloader = nullptr;

  /* Check for file changes */
  atools::util::FileSystemWatcher *fileWatcher = nullptr;

  bool verbose = false;

  /* Maps rounded altitude to wind layer data. Sorted by altitude. */
  QMap<int, WindAltLayer> windLayers;

};

QDebug operator<<(QDebug out, const atools::grib::Wind& wind);
QDebug operator<<(QDebug out, const atools::grib::WindPos& windPos);

} // namespace grib
} // namespace atools

Q_DECLARE_METATYPE(atools::grib::Wind);
Q_DECLARE_METATYPE(atools::grib::WindPos);

Q_DECLARE_TYPEINFO(atools::grib::Wind, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(atools::grib::WindPos, Q_PRIMITIVE_TYPE);

#endif // ATOOLS_GRIB_WINDQUERY_H
