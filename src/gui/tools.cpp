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

#include "gui/tools.h"
#include "gui/dialog.h"
#include "gui/helphandler.h"

#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QUrl>
#include <QDebug>
#include <QFontDatabase>
#include <QLabel>

#ifdef Q_OS_WIN32
#include <windows.h>
#endif

namespace atools {
namespace gui {

bool showInFileManager(const QString& filepath, QWidget *parent)
{
#ifdef Q_OS_WIN32
  QFileInfo fp(filepath);
  fp.makeAbsolute();

  // if(!QProcess::startDetached("explorer.exe", {"/select", QDir::toNativeSeparators(fp.filePath())},
  // QDir::homePath()))
  // QMessageBox::warning(mainWindow, QApplication::applicationName(), QString(
  // tr("Error starting explorer.exe with path \"%1\"")).
  // arg(query.queryItemValue("filepath")));

  if(fp.exists())
  {
    // Syntax is: explorer /select, "C:\Folder1\Folder2\file_to_select"
    // Dir separators MUST be win-style slashes

    // QProcess::startDetached() has an obscure bug. If the path has
    // no spaces and a comma(and maybe other special characters) it doesn't
    // get wrapped in quotes. So explorer.exe can't find the correct path and
    // displays the default one. If we wrap the path in quotes and pass it to
    // QProcess::startDetached() explorer.exe still shows the default path. In
    // this case QProcess::startDetached() probably puts its own quotes around ours.

    STARTUPINFO startupInfo;
    ::ZeroMemory(&startupInfo, sizeof(startupInfo));
    startupInfo.cb = sizeof(startupInfo);

    PROCESS_INFORMATION processInfo;
    ::ZeroMemory(&processInfo, sizeof(processInfo));

    QString nativePath(QDir::toNativeSeparators(fp.canonicalFilePath()));
    QString cmd = QString("explorer /select,\"%1\"").arg(nativePath);
    qDebug() << Q_FUNC_INFO << "command" << cmd;
    LPWSTR lpCmd = new WCHAR[cmd.size() + 1];
    cmd.toWCharArray(lpCmd);
    lpCmd[cmd.size()] = 0;

    bool ret = ::CreateProcessW(NULL, lpCmd, NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &processInfo);
    delete[] lpCmd;

    if(ret)
    {
      ::CloseHandle(processInfo.hProcess);
      ::CloseHandle(processInfo.hThread);
    }
    return true;
  }
  else
  {
    // If the item to select doesn't exist, try to open its parent
    QUrl url = QUrl::fromLocalFile(fp.path());
    if(!QDesktopServices::openUrl(url))
    {
      atools::gui::Dialog::warning(parent, QObject::tr("Error opening path \"%1\"").arg(filepath));
      return false;
    }
    else
      return true;
  }
#else
  QFileInfo fi(filepath);
  QUrl url = QUrl::fromLocalFile(fi.canonicalPath());
  qDebug() << Q_FUNC_INFO << "url" << url;

  if(!QDesktopServices::openUrl(url))
  {
    atools::gui::Dialog::warning(parent, QObject::tr("Error opening path \"%1\"").arg(filepath));
    return false;
  }
  else
    return true;

#endif
}

void anchorClicked(QWidget *parent, const QUrl& url)
{
  qDebug() << Q_FUNC_INFO << url;

  if(url.scheme() == "http" || url.scheme() == "https" || url.scheme() == "ftp")
    // Open a normal link from the userpoint description
    atools::gui::HelpHandler::openUrl(parent, url);
  else if(url.scheme() == "file")
  {
    if(url.isLocalFile())
    {
      QFileInfo info(url.toLocalFile());
      if(info.exists())
        // Open a file from the userpoint description
        atools::gui::HelpHandler::openUrl(parent, url);
      else
        atools::gui::Dialog::warning(parent, QObject::tr("File or directory \"%1\" does not exist.").
                                     arg(url.toDisplayString()));
    }
  }
}

void fontDescription(const QFont& font, QLabel *label)
{
  label->setFont(font);
  label->setText(fontDescription(font));
}

QString fontDescription(const QFont& font)
{
  QStringList fontText;

  fontText.append(font.family());
  if(font.pointSizeF() > 0.)
    fontText.append(QObject::tr("%1 pt").arg(font.pointSizeF()));
  else if(font.pixelSize() > 0)
    fontText.append(QObject::tr("%1 px").arg(font.pixelSize()));

  int weight = font.weight();
  if(weight == QFont::Thin)
    fontText.append(QObject::tr("thin"));
  else if(weight <= QFont::ExtraLight)
    fontText.append(QObject::tr("extra light"));
  else if(weight <= QFont::Light)
    fontText.append(QObject::tr("light"));
  else if(weight <= QFont::Normal)
    fontText.append(QObject::tr("normal"));
  else if(weight <= QFont::Medium)
    fontText.append(QObject::tr("medium"));
  else if(weight <= QFont::DemiBold)
    fontText.append(QObject::tr("demi bold"));
  else if(weight <= QFont::Bold)
    fontText.append(QObject::tr("bold"));
  else if(weight <= QFont::ExtraBold)
    fontText.append(QObject::tr("extra bold"));
  else if(weight >= QFont::Black)
    fontText.append(QObject::tr("black"));

  if(font.italic())
    fontText.append(QObject::tr("italic"));
  if(font.overline())
    fontText.append(QObject::tr("overline"));
  if(font.underline())
    fontText.append(QObject::tr("underline"));
  if(font.strikeOut())
    fontText.append(QObject::tr("strike out"));

  if(font.fixedPitch())
    fontText.append(QObject::tr("fixed pitch"));

  QString prefix;
  if(font == QFontDatabase::systemFont(QFontDatabase::GeneralFont))
    prefix = QObject::tr("System font: %1");
  else
    prefix = QObject::tr("User selected font: %1");

  return prefix.arg(fontText.join(QObject::tr(", ")));
}

} // namespace gui
} // namespace atools
