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

#include "gui/signalblocker.h"

#include <QAction>

namespace atools {
namespace gui {

SignalBlocker::SignalBlocker(QList<QObject *> objectList)
{
  add(objectList);
}

SignalBlocker::~SignalBlocker()
{
  for(QObject *object : objects)
  {
    if(object != nullptr)
      object->blockSignals(false);
  }
}

void SignalBlocker::add(QObject *object)
{
  if(object != nullptr)
  {
    objects.append(object);
    object->blockSignals(true);
  }
}

} // namespace gui
} // namespace atools
