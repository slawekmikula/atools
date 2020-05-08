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

#ifndef ATOOLS_GUI_TOOLS_H
#define ATOOLS_GUI_TOOLS_H

class QString;
class QWidget;
class QUrl;

namespace atools {
namespace gui {

/* Show path in any OS dependent file manager. Selects the file in Windows Explorer.
 *  Shows a warning dialog on error.*/
bool showInFileManager(const QString& filepath, QWidget *parent);

/* Open HTTP, HTTPS, FTP or FILE link in browser or file manager */
void anchorClicked(QWidget *parent, const QUrl& url);

} // namespace gui
} // namespace atools

#endif // ATOOLS_GUI_TOOLS_H
