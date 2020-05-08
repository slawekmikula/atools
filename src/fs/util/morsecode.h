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

#ifndef ATOOLS_FS_UTIL_MORSECODE_H
#define ATOOLS_FS_UTIL_MORSECODE_H

#include <QString>

namespace atools {
namespace fs {
namespace util {

/*
 * Converts strings to a morse code string.
 */
class MorseCode
{
public:
  /*
   * @param signSeparator separator character for each dot/dash
   * @param charSeparator separator character for each sign
   */
  MorseCode(const QString& signSeparator = QString(), const QString& charSeparator = "\n");
  ~MorseCode();

  QString getCode(const QString& text);

private:
  QString signSep, charSep;
};

} // namespace util
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_UTIL_MORSECODE_H
