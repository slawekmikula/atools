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

#include "fs/weather/xpweatherreader.h"

#include "util/filesystemwatcher.h"
#include "fs/weather/metarindex.h"

#include <QFileInfo>
#include <QRegularExpression>
#include <QTextStream>
#include <QDebug>

namespace atools {
namespace fs {
namespace weather {

using atools::util::FileSystemWatcher;

XpWeatherReader::XpWeatherReader(QObject *parent, int indexSize, bool verboseLogging)
  : QObject(parent), verbose(verboseLogging)
{
  index = new atools::fs::weather::MetarIndex(indexSize, true /* xplane */, verboseLogging);
}

XpWeatherReader::~XpWeatherReader()
{
  clear();
  delete index;
}

void XpWeatherReader::setWeatherFile(const QString& file)
{
  clear();
  weatherFile = file;
}

void XpWeatherReader::readWeatherFile()
{
  // File set and not watching a file already
  if(!weatherFile.isEmpty() && fsWatcher == nullptr)
  {
    qDebug() << Q_FUNC_INFO << weatherFile;

    index->clear();
    deleteFsWatcher();

    createFsWatcher();

    QFileInfo fileinfo(weatherFile);
    if(fileinfo.exists() && fileinfo.isFile())
      read();
    // else wait for file created
  }
}

void XpWeatherReader::clear()
{
  qDebug() << Q_FUNC_INFO;
  deleteFsWatcher();
  index->clear();
  weatherFile.clear();
}

void XpWeatherReader::setFetchAirportCoords(const std::function<geo::Pos(const QString&)>& value)
{
  index->setFetchAirportCoords(value);
}

bool XpWeatherReader::read()
{
  bool retval = false;
  QFile file(weatherFile);
  if(file.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    qDebug() << Q_FUNC_INFO << weatherFile;
    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    retval = index->read(stream, weatherFile, false /* merge */);
    file.close();
  }
  else
    qWarning() << "cannot open" << file.fileName() << "reason" << file.errorString();
  return retval;
}

atools::fs::weather::MetarResult XpWeatherReader::getXplaneMetar(const QString& station, const atools::geo::Pos& pos)
{
  readWeatherFile();
  return index->getMetar(station, pos);
}

QSet<QString> XpWeatherReader::getMetarAirportIdents()
{
  readWeatherFile();
  return index->getMetarAirportIdents();
}

QString XpWeatherReader::getMetar(const QString& ident)
{
  readWeatherFile();
  return index->getMetar(ident);
}

void XpWeatherReader::pathChanged(const QString& filename)
{
  if(verbose)
    qDebug() << Q_FUNC_INFO << filename;

  QFileInfo fileinfo(weatherFile);
  if(fileinfo.exists() && fileinfo.isFile())
  {
    if(verbose)
      qDebug() << Q_FUNC_INFO << "File exists" << fileinfo.exists()
               << "size" << fileinfo.size() << "last modified" << fileinfo.lastModified();

    qDebug() << Q_FUNC_INFO << "reading" << weatherFile;
    if(read())
      emit weatherUpdated();
    else if(verbose)
      qDebug() << Q_FUNC_INFO << "File not changed";
  }
  else
    // File was deleted - keep current weather information
    qDebug() << Q_FUNC_INFO << "File does not exist. Index empty:" << index->isEmpty();
}

void XpWeatherReader::deleteFsWatcher()
{
  if(fsWatcher != nullptr)
  {
    fsWatcher->disconnect(fsWatcher, &FileSystemWatcher::fileUpdated, this, &XpWeatherReader::pathChanged);
    fsWatcher->deleteLater();
    fsWatcher = nullptr;
  }
}

void XpWeatherReader::createFsWatcher()
{
  if(fsWatcher == nullptr)
  {
    // Watch file for changes and directory too to catch file deletions
    fsWatcher = new FileSystemWatcher(this, verbose);

    // Set to smaller value to deal with ASX weather files
    fsWatcher->setMinFileSize(1000);
    fsWatcher->connect(fsWatcher, &FileSystemWatcher::fileUpdated, this, &XpWeatherReader::pathChanged);
  }

  fsWatcher->setFilenameAndStart(weatherFile);
}

} // namespace weather
} // namespace fs
} // namespace atools
