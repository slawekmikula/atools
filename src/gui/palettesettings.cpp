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

#include "gui/palettesettings.h"

#include <QSettings>
#include <QPalette>
#include <QApplication>

namespace atools {
namespace gui {

static QMap<QString, QPalette::ColorGroup> GROUP_NAME_MAP(
    {
      {"Active", QPalette::Active},
      {"Disabled", QPalette::Disabled},
      {"Inactive", QPalette::Inactive}
    }
  );

static QMap<QString, QPalette::ColorRole> ROLE_NAME_MAP(
    {
      {"WindowText", QPalette::WindowText},
      {"Button", QPalette::Button},
      {"Light", QPalette::Light},
      {"Midlight", QPalette::Midlight},
      {"Dark", QPalette::Dark},
      {"Mid", QPalette::Mid},
      {"Text", QPalette::Text},
      {"BrightText", QPalette::BrightText},
      {"ButtonText", QPalette::ButtonText},
      {"Base", QPalette::Base},
      {"Window", QPalette::Window},
      {"Shadow", QPalette::Shadow},
      {"Highlight", QPalette::Highlight},
      {"HighlightedText", QPalette::HighlightedText},
      {"Link", QPalette::Link},
      {"LinkVisited", QPalette::LinkVisited},
      {"AlternateBase", QPalette::AlternateBase},
      {"NoRole", QPalette::NoRole},
      {"ToolTipBase", QPalette::ToolTipBase},
      {"ToolTipText", QPalette::ToolTipText}
    }
  );

PaletteSettings::PaletteSettings(const QString& settingsFile, const QString& groupName, const QString& keyPrefix)
  : group(groupName), prefix(keyPrefix)
{
  settings = new QSettings(settingsFile, QSettings::IniFormat);
}

PaletteSettings::~PaletteSettings()
{
  delete settings;
}

void PaletteSettings::syncPalette(QPalette& palette)
{
  settings->setValue("Options/Version", QApplication::applicationVersion());

  settings->beginGroup(group);
  for(const QString& groupKey : GROUP_NAME_MAP.keys())
  {
    for(const QString& roleKey : ROLE_NAME_MAP.keys())
    {
      QString key(prefix + groupKey + "_" + roleKey);
      if(settings->contains(key))
        palette.setColor(GROUP_NAME_MAP.value(groupKey), ROLE_NAME_MAP.value(roleKey),
                         QColor(settings->value(key).toString()));
      else
        settings->setValue(key, palette.color(GROUP_NAME_MAP.value(groupKey), ROLE_NAME_MAP.value(roleKey)).name());
    }
  }
  settings->endGroup();
  settings->sync();
}

void PaletteSettings::savePalette(const QPalette& palette)
{
  settings->beginGroup(group);
  for(const QString& groupKey : GROUP_NAME_MAP.keys())
  {
    for(const QString& roleKey : ROLE_NAME_MAP.keys())
    {
      QString key(prefix + groupKey + "_" + roleKey);
      settings->setValue(key, palette.color(GROUP_NAME_MAP.value(groupKey), ROLE_NAME_MAP.value(roleKey)).name());
    }
  }
  settings->endGroup();
  settings->sync();
}

void PaletteSettings::loadPalette(QPalette& palette)
{
  settings->beginGroup(group);
  for(const QString& groupKey : GROUP_NAME_MAP.keys())
  {
    for(const QString& roleKey : ROLE_NAME_MAP.keys())
    {
      QString key(prefix + groupKey + "_" + roleKey);
      if(settings->contains(key))
        palette.setColor(GROUP_NAME_MAP.value(groupKey), ROLE_NAME_MAP.value(roleKey),
                         QColor(settings->value(key).toString()));
    }
  }
  settings->endGroup();
  settings->sync();
}

} // namespace gui
} // namespace atools
