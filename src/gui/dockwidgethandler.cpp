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

#include "gui/dockwidgethandler.h"

#include "exception.h"

#include <QAction>
#include <QDockWidget>
#include <QMainWindow>
#include <QDebug>
#include <QEvent>
#include <QMouseEvent>
#include <QStatusBar>
#include <QMenuBar>
#include <QToolBar>
#include <QMessageBox>
#include <QApplication>
#include <QScreen>

namespace atools {
namespace gui {

/* Do not restore these states */
const static Qt::WindowStates WINDOW_STATE_MASK =
  ~(Qt::WindowStates(Qt::WindowMinimized) | Qt::WindowStates(Qt::WindowActive));

/* Saves the main window states and states of all attached widgets like the status bars and the menu bar. */
struct MainWindowState
{
  explicit MainWindowState(QMainWindow *mainWindow)
  {
    fromWindow(mainWindow);
  }

  MainWindowState()
  {
  }

  /* Copy state to main window and all related widgets. Saved position to place fullscreen and maximized windows
   * is used if position is null. null = use current screen, otherwise use save screen position. */
  void toWindow(QMainWindow *mainWindow, const QPoint *position) const;

  /* Save state from main window and all related widgets */
  void fromWindow(const QMainWindow *mainWindow);

  /* Create an initial fullscreen configuration without docks and toobars depending on configuration */
  void initFullscreen(atools::gui::DockFlags flags);

  /* Clear all and set valid to false */
  void clear();

  /* false if default constructed or cleared */
  bool isValid() const
  {
    return valid;
  }

  QByteArray mainWindowState; // State from main window including toolbars and dock widgets
  QSize mainWindowSize;
  QPoint mainWindowPosition;
  Qt::WindowStates mainWindowStates = Qt::WindowNoState;

  bool statusBarVisible = true, valid = false, verbose = false;
};

QDebug operator<<(QDebug out, const MainWindowState& obj)
{
  QDebugStateSaver saver(out);
  out.noquote().nospace() << "MainWindowState["
                          << "size " << obj.mainWindowState.size()
                          << ", window size " << obj.mainWindowSize
                          << ", window position " << obj.mainWindowPosition
                          << ", window states " << obj.mainWindowStates
                          << ", statusbar " << obj.statusBarVisible
                          << ", valid " << obj.valid
                          << "]";
  return out;
}

void MainWindowState::toWindow(QMainWindow *mainWindow, const QPoint *position) const
{
  if(verbose)
    qDebug() << Q_FUNC_INFO << *this;

  if(!valid)
    qWarning() << Q_FUNC_INFO << "Calling on invalid state";

  if(mainWindowStates.testFlag(Qt::WindowMaximized) || mainWindowStates.testFlag(Qt::WindowFullScreen))
    // Move window to position before going fullscreen or maximized to catch right screen
    mainWindow->move(position == nullptr ? mainWindowPosition : *position);

  // Set normal, maximized or fullscreen
  mainWindow->setWindowState(mainWindowStates & WINDOW_STATE_MASK);

  if(!mainWindowStates.testFlag(Qt::WindowMaximized) && !mainWindowStates.testFlag(Qt::WindowFullScreen))
  {
    // Change size and position only if main window is not maximized or full screen
    if(mainWindowSize.isValid())
      mainWindow->resize(mainWindowSize);
    mainWindow->move(mainWindowPosition);
  }
  if(mainWindow->statusBar() != nullptr)
    mainWindow->statusBar()->setVisible(statusBarVisible);

  // Restores the state of this mainwindow's toolbars and dockwidgets. Also restores the corner settings too.
  // Has to be called after setting size to avoid unwanted widget resizing
  if(!mainWindowState.isEmpty())
    mainWindow->restoreState(mainWindowState);

  if(mainWindow->menuWidget() != nullptr)
    mainWindow->menuWidget()->setVisible(true); // Do not hide
}

void MainWindowState::fromWindow(const QMainWindow *mainWindow)
{
  clear();
  mainWindowState = mainWindow->saveState();
  mainWindowSize = mainWindow->size();
  mainWindowPosition = mainWindow->pos();
  mainWindowStates = mainWindow->windowState();
  statusBarVisible = mainWindow->statusBar()->isVisible();
  valid = true;

  if(verbose)
    qDebug() << Q_FUNC_INFO << *this;
}

void MainWindowState::initFullscreen(atools::gui::DockFlags flags)
{
  clear();

  mainWindowStates = flags.testFlag(MAXIMIZE) ? Qt::WindowMaximized : Qt::WindowFullScreen;
  statusBarVisible = !flags.testFlag(HIDE_STATUSBAR);
  valid = true;

  if(verbose)
    qDebug() << Q_FUNC_INFO << *this;
}

void MainWindowState::clear()
{
  mainWindowState.clear();
  mainWindowSize = QSize();
  mainWindowPosition = QPoint();
  mainWindowStates = Qt::WindowNoState;
  statusBarVisible = true,
  valid = false;
}

QDataStream& operator<<(QDataStream& out, const atools::gui::MainWindowState& state)
{
  bool menuVisible = true;
  out << state.valid << state.mainWindowState << state.mainWindowSize << state.mainWindowPosition
      << state.mainWindowStates << state.statusBarVisible << menuVisible;
  return out;
}

QDataStream& operator>>(QDataStream& in, atools::gui::MainWindowState& state)
{
  bool menuVisible;
  in >> state.valid >> state.mainWindowState >> state.mainWindowSize >> state.mainWindowPosition
  >> state.mainWindowStates >> state.statusBarVisible >> menuVisible;
  return in;
}

// ===================================================================================
class DockEventFilter :
  public QObject
{
public:
  DockEventFilter()
  {

  }

  bool autoRaiseDockWindow = false, autoRaiseMainWindow = false;

private:
  virtual bool eventFilter(QObject *object, QEvent *event) override;

};

bool DockEventFilter::eventFilter(QObject *object, QEvent *event)
{
  if(event->type() == QEvent::Enter)
  {
    if(autoRaiseDockWindow)
    {
      QDockWidget *widget = dynamic_cast<QDockWidget *>(object);
      if(widget != nullptr)
      {
        qDebug() << Q_FUNC_INFO << event->type() << widget->objectName();
        if(widget->isFloating())
        {
          widget->activateWindow();
          widget->raise();
        }
      }
    }

    if(autoRaiseMainWindow)
    {
      QMainWindow *mainWindow = dynamic_cast<QMainWindow *>(object);
      if(mainWindow != nullptr)
      {
        mainWindow->activateWindow();
        mainWindow->raise();
      }
    }
  }

  return QObject::eventFilter(object, event);
}

// ===================================================================================
DockWidgetHandler::DockWidgetHandler(QMainWindow *parentMainWindow, const QList<QDockWidget *>& dockWidgetsParam,
                                     const QList<QToolBar *>& toolBarsParam, bool verboseLog)
  : QObject(parentMainWindow), mainWindow(parentMainWindow), dockWidgets(dockWidgetsParam), toolBars(toolBarsParam),
  verbose(verboseLog)
{
  dockEventFilter = new DockEventFilter();
  normalState = new MainWindowState;
  normalState->verbose = verbose;
  fullscreenState = new MainWindowState;
  fullscreenState->verbose = verbose;
}

DockWidgetHandler::~DockWidgetHandler()
{
  delete dockEventFilter;
  delete normalState;
  delete fullscreenState;
}

void DockWidgetHandler::dockVisibilityChanged(bool visible)
{
  if(verbose)
    qDebug() << Q_FUNC_INFO << "visible" << visible;

  if(visible)
  {
    QDockWidget *dockWidget = dynamic_cast<QDockWidget *>(sender());
    if(dockWidget != nullptr)
    {
      if(dockWidget->isFloating())
      {
        // Check if widget or its title bar are off screen and correct position if needed
        QPoint pos = dockWidget->pos();
        if(pos.y() < 0)
          pos.setY(10);
        if(pos.x() < 0)
          pos.setX(10);
        if(pos != dockWidget->pos())
        {
          qDebug() << Q_FUNC_INFO << "Correcting dock position for" << dockWidget->objectName()
                   << "from" << dockWidget->pos() << "to" << pos;
          dockWidget->move(pos);
        }
      }
    }
  }
}

void DockWidgetHandler::dockTopLevelChanged(bool topLevel)
{
  if(verbose)
    qDebug() << Q_FUNC_INFO;

  Q_UNUSED(topLevel)
  updateDockTabStatus();
}

void DockWidgetHandler::dockLocationChanged(Qt::DockWidgetArea area)
{
  if(verbose)
    qDebug() << Q_FUNC_INFO;

  Q_UNUSED(area)
  updateDockTabStatus();
}

void DockWidgetHandler::connectDockWindow(QDockWidget *dockWidget)
{
  updateDockTabStatus();
  connect(dockWidget->toggleViewAction(), &QAction::toggled, this, &DockWidgetHandler::dockViewToggled);
  connect(dockWidget, &QDockWidget::dockLocationChanged, this, &DockWidgetHandler::dockLocationChanged);
  connect(dockWidget, &QDockWidget::topLevelChanged, this, &DockWidgetHandler::dockTopLevelChanged);
  connect(dockWidget, &QDockWidget::visibilityChanged, this, &DockWidgetHandler::dockVisibilityChanged);
  dockWidget->installEventFilter(dockEventFilter);
}

void DockWidgetHandler::toggledDockWindow(QDockWidget *dockWidget, bool checked)
{
  bool handle = handleDockViews;

  // Do not remember stacks triggered by signals
  handleDockViews = false;

  if(checked)
  {
    // Find a stack that contains the widget ==================
    auto it = std::find_if(dockStackList.begin(), dockStackList.end(),
                           [dockWidget](QList<QDockWidget *>& list)
        {
          return list.contains(dockWidget);
        });

    if(it != dockStackList.end())
    {
      // Found a stack now show all stack member widgets
      for(QDockWidget *dock : *it)
      {
        if(dock != dockWidget)
          dock->show();
      }
    }

    // Show the widget whose action fired
    dockWidget->show();
    dockWidget->activateWindow();
    dockWidget->raise();
  }
  else
  {
    // Even floating widgets can have tabified buddies - ignore floating
    if(!dockWidget->isFloating())
    {
      for(QDockWidget *dock : mainWindow->tabifiedDockWidgets(dockWidget))
      {
        if(!dock->isFloating())
          dock->close();
      }
    }
  }
  handleDockViews = handle;
}

void DockWidgetHandler::updateDockTabStatus()
{
  if(handleDockViews)
  {
    dockStackList.clear();
    for(QDockWidget *dock : dockWidgets)
      updateDockTabStatus(dock);
  }
}

void DockWidgetHandler::updateDockTabStatus(QDockWidget *dockWidget)
{
  if(dockWidget->isFloating())
    return;

  QList<QDockWidget *> tabified = mainWindow->tabifiedDockWidgets(dockWidget);
  if(!tabified.isEmpty())
  {
    auto it = std::find_if(dockStackList.begin(), dockStackList.end(), [dockWidget](QList<QDockWidget *>& list) -> bool
        {
          return list.contains(dockWidget);
        });

    if(it == dockStackList.end())
    {
      auto rmIt = std::remove_if(tabified.begin(), tabified.end(), [](QDockWidget *dock) -> bool
          {
            return dock->isFloating();
          });
      if(rmIt != tabified.end())
        tabified.erase(rmIt, tabified.end());

      if(!tabified.isEmpty())
      {
        tabified.append(dockWidget);
        dockStackList.append(tabified);
      }
    }
  }
}

void DockWidgetHandler::dockViewToggled()
{
  if(verbose)
    qDebug() << Q_FUNC_INFO;

  if(handleDockViews)
  {
    QAction *action = dynamic_cast<QAction *>(sender());
    if(action != nullptr)
    {
      bool checked = action->isChecked();
      for(QDockWidget *dock : dockWidgets)
      {
        if(action == dock->toggleViewAction())
          toggledDockWindow(dock, checked);
      }
    }
  }
}

void DockWidgetHandler::activateWindow(QDockWidget *dockWidget)
{
  qDebug() << Q_FUNC_INFO;
  dockWidget->show();
  dockWidget->activateWindow();
  dockWidget->raise();
}

void DockWidgetHandler::setHandleDockViews(bool value)
{
  handleDockViews = value;
  updateDockTabStatus();
}

bool DockWidgetHandler::isAutoRaiseDockWindows() const
{
  return dockEventFilter->autoRaiseDockWindow;
}

void DockWidgetHandler::setAutoRaiseDockWindows(bool value)
{
  dockEventFilter->autoRaiseDockWindow = value;
}

bool DockWidgetHandler::isAutoRaiseMainWindow() const
{
  return dockEventFilter->autoRaiseMainWindow;
}

void DockWidgetHandler::setAutoRaiseMainWindow(bool value)
{
  dockEventFilter->autoRaiseMainWindow = value;
}

void DockWidgetHandler::setStayOnTopMain(bool value) const
{
  setStayOnTop(mainWindow, value);

  for(QDockWidget *dock : dockWidgets)
  {
    if(dock->isFloating())
      setStayOnTop(dock, value);
  }
}

bool DockWidgetHandler::isStayOnTopMain() const
{
  return isStayOnTop(mainWindow);
}

void DockWidgetHandler::setStayOnTop(QWidget *window, bool value) const
{
  if(window->windowFlags().testFlag(Qt::WindowStaysOnTopHint) != value)
  {
    bool visible = window->isVisible();

    window->setWindowFlag(Qt::WindowStaysOnTopHint, value);

    if(visible)
      // Need to reopen window since changing window flags closes window
      window->show();
  }
}

bool DockWidgetHandler::isStayOnTop(QWidget *window) const
{
  return window->windowFlags().testFlag(Qt::WindowStaysOnTopHint);
}

void DockWidgetHandler::setDockingAllowed(bool value)
{
  if(allowedAreas.isEmpty())
  {
    // Create backup
    for(QDockWidget *dock : dockWidgets)
      allowedAreas.append(dock->allowedAreas());
  }

  if(value)
  {
    // Restore backup
    for(int i = 0; i < dockWidgets.size(); i++)
      dockWidgets[i]->setAllowedAreas(allowedAreas.value(i, Qt::AllDockWidgetAreas));
  }
  else
  {
    // Forbid docking for all widgets
    for(QDockWidget *dock : dockWidgets)
      dock->setAllowedAreas(value ? Qt::AllDockWidgetAreas : Qt::NoDockWidgetArea);
  }
}

void DockWidgetHandler::raiseFloatingWindow(QDockWidget *dockWidget)
{
  qDebug() << Q_FUNC_INFO;
  if(dockWidget->isVisible() && dockWidget->isFloating())
    dockWidget->raise();
}

void DockWidgetHandler::connectDockWindows()
{
  for(QDockWidget *dock : dockWidgets)
    connectDockWindow(dock);
  mainWindow->installEventFilter(dockEventFilter);
}

void DockWidgetHandler::raiseFloatingWindows()
{
  for(QDockWidget *dock : dockWidgets)
    raiseFloatingWindow(dock);
}

// ==========================================================================
// Fullscreen methods

void DockWidgetHandler::setFullScreenOn(atools::gui::DockFlags flags)
{
  if(!fullscreen)
  {
    if(verbose)
      qDebug() << Q_FUNC_INFO;

    // Copy window layout to state
    normalState->fromWindow(mainWindow);

    if(!fullscreenState->isValid())
    {
      // No saved fullscreen configuration yet - create a new one
      fullscreenState->initFullscreen(flags);

      if(flags.testFlag(HIDE_TOOLBARS))
      {
        for(QToolBar *toolBar : toolBars)
          toolBar->setVisible(false);
      }

      if(flags.testFlag(HIDE_DOCKS))
      {
        for(QDockWidget *dockWidget : dockWidgets)
          dockWidget->setVisible(false);
      }
    }

    // Main window to fullscreen - keep window on same screen
    fullscreenState->toWindow(mainWindow, &normalState->mainWindowPosition);

    fullscreen = true;
    delayedFullscreen = false;
  }
  else
    qWarning() << Q_FUNC_INFO << "Already fullscreen";
}

void DockWidgetHandler::setFullScreenOff()
{
  if(fullscreen)
  {
    if(verbose)
      qDebug() << Q_FUNC_INFO;

    // Save full screen layout
    fullscreenState->fromWindow(mainWindow);

    // Assign normal state to window and keep window on same screen
    normalState->toWindow(mainWindow, &fullscreenState->mainWindowPosition);

    fullscreen = false;
    delayedFullscreen = false;
  }
  else
    qWarning() << Q_FUNC_INFO << "Already no fullscreen";
}

QByteArray DockWidgetHandler::saveState()
{
  // Save current state - other state was saved when switching fs/normal
  if(fullscreen)
    fullscreenState->fromWindow(mainWindow);
  else
    normalState->fromWindow(mainWindow);

  qDebug() << Q_FUNC_INFO << "normalState" << *normalState;
  qDebug() << Q_FUNC_INFO << "fullscreenState" << *fullscreenState;

  // Save states for each mode and also fullscreen status
  QByteArray data;
  QDataStream stream(&data, QIODevice::WriteOnly);
  stream << fullscreen << *normalState << *fullscreenState;
  return data;
}

void DockWidgetHandler::restoreState(QByteArray data)
{
  QDataStream stream(&data, QIODevice::ReadOnly);
  stream >> fullscreen >> *normalState >> *fullscreenState;
  delayedFullscreen = false;

  qDebug() << Q_FUNC_INFO << "normalState" << *normalState;
  qDebug() << Q_FUNC_INFO << "fullscreenState" << *fullscreenState;
}

void DockWidgetHandler::currentStateToWindow()
{
  if(verbose)
    qDebug() << Q_FUNC_INFO;

  if(fullscreen)
    fullscreenState->toWindow(mainWindow, nullptr);
  else
    normalState->toWindow(mainWindow, nullptr);
}

void DockWidgetHandler::normalStateToWindow()
{
  normalState->toWindow(mainWindow, nullptr);
  delayedFullscreen = fullscreen; // Set flag to allow switch to fullscreen later after showing windows
  fullscreen = false;
}

void DockWidgetHandler::fullscreenStateToWindow()
{
  fullscreenState->toWindow(mainWindow, nullptr);
  fullscreen = true;
  delayedFullscreen = false;
}

void DockWidgetHandler::resetWindowState(const QSize& size, const QString& filename)
{
  QFile file(filename);
  if(file.open(QIODevice::ReadOnly))
  {
    QByteArray bytes = file.readAll();

    if(!bytes.isEmpty())
    {
      qDebug() << Q_FUNC_INFO;

      // Reset also ends fullscreen mode
      fullscreen = false;

      // End maximized and fullscreen state
      mainWindow->setWindowState(Qt::WindowNoState);

      // Move to origin and apply size
      mainWindow->move(QGuiApplication::primaryScreen()->availableGeometry().topLeft());
      mainWindow->resize(size);

      // Reload state now. This has to be done after resizing the window.
      mainWindow->restoreState(bytes);

      normalState->fromWindow(mainWindow);
      fullscreenState->clear();

      if(mainWindow->menuWidget() != nullptr)
        mainWindow->menuWidget()->setVisible(true); // Do not hide
    }
    else
      throw atools::Exception(tr("Error reading \"%1\": %2").arg(filename).arg(file.errorString()));

    file.close();
  }
  else
    throw atools::Exception(tr("Error reading \"%1\": %2").arg(filename).arg(file.errorString()));
}

void DockWidgetHandler::saveWindowState(const QString& filename, bool allowUndockCentral)
{
  qDebug() << Q_FUNC_INFO << filename;

  QFile file(filename);
  if(file.open(QIODevice::WriteOnly))
  {
    // Copy current window status to slot
    if(fullscreen)
      fullscreenState->fromWindow(mainWindow);
    else
      normalState->fromWindow(mainWindow);

    // Save all to stream
    QDataStream stream(&file);
    stream << FILE_MAGIC_NUMBER << FILE_VERSION
           << allowUndockCentral << fullscreen
           << *normalState << *fullscreenState;

    if(file.error() != QFileDevice::NoError)
      throw atools::Exception(tr("Error writing \"%1\": %2").arg(filename).arg(file.errorString()));

    file.close();
  }
  else
    throw atools::Exception(tr("Error writing \"%1\": %2").arg(filename).arg(file.errorString()));
}

bool DockWidgetHandler::loadWindowState(const QString& filename, bool allowUndockCentral,
                                        const QString& allowUndockCentralErrorMessage)
{
  qDebug() << Q_FUNC_INFO << filename;
  QFile file(filename);
  if(file.open(QIODevice::ReadOnly))
  {
    QDataStream stream(&file);

    // Read and check magic number and version =================
    quint32 magicNumber;
    quint16 version;
    stream >> magicNumber >> version;

    if(magicNumber != FILE_MAGIC_NUMBER)
      throw atools::Exception(tr("Error reading \"%1\": Invalid magic number. Not a window layout file.").
                              arg(filename));
    if(version != FILE_VERSION)
      throw atools::Exception(tr("Error reading \"%1\": Invalid version. Incompatible window layout file.").
                              arg(filename));

    // Read all into temporary variables ===============
    bool fs, allowUndock = allowUndockCentral;
    MainWindowState normal, full;
    stream >> allowUndock >> fs >> normal >> full;

    if(file.error() != QFileDevice::NoError)
      throw atools::Exception("Error reading \"" + filename + "\": " + file.errorString());

    file.close();

    int retval = QMessageBox::Yes;
    if(allowUndock != allowUndockCentral)
      // A layout file can only be applied properly if the state of the central widget (normal or dock widget)
      // is the same - show warning
      retval = QMessageBox::question(mainWindow, QApplication::applicationName(),
                                     allowUndockCentralErrorMessage, QMessageBox::Yes | QMessageBox::Cancel);

    if(retval == QMessageBox::Yes)
    {
      // Copy temporary variables to fields
      // fullscreen = fs; // leave this up to the application
      *normalState = normal;
      *fullscreenState = full;

      if(verbose)
      {
        qDebug() << Q_FUNC_INFO << "normalState" << *normalState;
        qDebug() << Q_FUNC_INFO << "fullscreenState" << *fullscreenState;
      }

      return true;
    }
  }
  else
    throw atools::Exception(tr("Error reading \"%1\": %2").arg(filename).arg(file.errorString()));

  // nothing to apply
  return false;
}

bool DockWidgetHandler::isWindowLayoutFile(const QString& filename)
{
  qDebug() << Q_FUNC_INFO << filename;
  QFile file(filename);
  if(file.open(QIODevice::ReadOnly))
  {
    QDataStream stream(&file);

    // Read and check magic number and version =================
    quint32 magicNumber = 0;
    quint16 version = 0;
    stream >> magicNumber >> version;

    bool ok = magicNumber == FILE_MAGIC_NUMBER && version == FILE_VERSION && file.error() == QFileDevice::NoError;

    file.close();

    return ok;
  }
  return false;
}

void DockWidgetHandler::registerMetaTypes()
{
  qRegisterMetaTypeStreamOperators<atools::gui::MainWindowState>();
}

} // namespace gui
} // namespace atools

// Enable use in QVariant
Q_DECLARE_METATYPE(atools::gui::MainWindowState);
