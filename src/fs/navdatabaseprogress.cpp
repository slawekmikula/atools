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

#include "fs/navdatabaseprogress.h"
#include "fs/scenery/sceneryarea.h"

#include <QFileInfo>

namespace atools {
namespace fs {

NavDatabaseProgress::NavDatabaseProgress()
{

}

const QString& NavDatabaseProgress::getSceneryTitle() const
{
  return sceneryArea->getTitle();
}

const QString& NavDatabaseProgress::getSceneryPath() const
{
  return sceneryArea->getLocalPath();
}

QString NavDatabaseProgress::getBglFileName() const
{
  return QFileInfo(bglFilepath).fileName();
}



} // namespace fs
} // namespace atools
