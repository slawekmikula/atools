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

#include "gui/tabwidgethandler.h"

#include "settings/settings.h"

#include <QTabWidget>
#include <QTabBar>
#include <QToolButton>
#include <QAction>
#include <QDebug>
#include <QLabel>
#include <QMenu>
#include <QApplication>
#include <QStyle>

const static char ID_PROPERTY[] = "tabid";

namespace atools {
namespace gui {

TabWidgetHandler::TabWidgetHandler(QTabWidget *tabWidgetParam, const QIcon& icon, const QString& toolButtonTooltip)
  : QObject(tabWidgetParam), tabWidget(tabWidgetParam)
{
  styleChanged();

  connect(tabWidget, &QTabWidget::tabCloseRequested, this, &TabWidgetHandler::tabCloseRequested);
  connect(tabWidget, &QTabWidget::currentChanged, this, &TabWidgetHandler::currentChanged);

  // Create tool button =================================================
  toolButtonCorner = new QToolButton(tabWidget);
  toolButtonCorner->setIcon(icon);
  toolButtonCorner->setToolTip(toolButtonTooltip);
  toolButtonCorner->setStatusTip(toolButtonTooltip);
  toolButtonCorner->setPopupMode(QToolButton::InstantPopup);
  toolButtonCorner->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  tabWidget->setCornerWidget(toolButtonCorner);

  // Create and add select all action =====================================
  actionAll = new QAction(tr("&Open All"), toolButtonCorner);
  actionAll->setToolTip(tr("Show all tabs"));
  actionAll->setStatusTip(actionAll->toolTip());
  toolButtonCorner->addAction(actionAll);
  connect(actionAll, &QAction::triggered, this, &TabWidgetHandler::toolbarActionTriggered);

  // Create and add select none action =====================================
  actionNone = new QAction(tr("&Close All Except Current"), toolButtonCorner);
  actionNone->setToolTip(tr("Close all tabs except the current tab"));
  actionNone->setStatusTip(actionNone->toolTip());
  toolButtonCorner->addAction(actionNone);
  connect(actionNone, &QAction::triggered, this, &TabWidgetHandler::toolbarActionTriggered);

  // Create and add reset action =====================================
  actionReset = new QAction(tr("&Reset Tab Layout"), toolButtonCorner);
  actionReset->setToolTip(tr("Show all tabs and reset order back to default"));
  actionReset->setStatusTip(actionReset->toolTip());
  toolButtonCorner->addAction(actionReset);
  connect(actionReset, &QAction::triggered, this, &TabWidgetHandler::toolbarActionTriggered);

  // Create and add lock action =====================================
  actionLock = new QAction(tr("&Lock Tab Layout"), toolButtonCorner);
  actionLock->setToolTip(tr("Hides close buttons and fixes tabs at current position"));
  actionLock->setStatusTip(actionLock->toolTip());
  actionLock->setCheckable(true);
  toolButtonCorner->addAction(actionLock);
  connect(actionLock, &QAction::toggled, this, &TabWidgetHandler::toolbarActionTriggered);

  // Enable and connect context menu
  tabWidget->tabBar()->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(tabWidget->tabBar(), &QTabBar::customContextMenuRequested, this, &TabWidgetHandler::tableContextMenu);

  // Enable close on double click
  connect(tabWidget, &QTabWidget::tabBarDoubleClicked, this, &TabWidgetHandler::tabCloseRequested);
}

TabWidgetHandler::~TabWidgetHandler()
{
  clear();
  delete actionLock;
  delete actionReset;
  delete actionNone;
  delete actionAll;
  delete toolButtonCorner;
}

void TabWidgetHandler::clear()
{
  for(const Tab& tab : tabs)
  {
    toolButtonCorner->removeAction(tab.action);
    delete tab.action;
  }
  tabs.clear();
}

void TabWidgetHandler::reset()
{
  {
    QSignalBlocker blocker(tabWidget);
    resetInternal();

    QSignalBlocker blocker2(actionLock);
    actionLock->setChecked(false);
  }
  updateWidgets();
  updateTabs();

}

void TabWidgetHandler::resetInternal()
{
  clearTabWidget();
  for(const Tab& tab : tabs)
    addTab(tab.action->data().toInt());

  // Activate first
  tabWidget->setCurrentIndex(0);
}

void TabWidgetHandler::tableContextMenu(const QPoint& pos)
{
  QPoint menuPos = QCursor::pos();
  // Move menu position off the cursor to avoid accidental selection on touchpads
  menuPos += QPoint(3, 3);

  QMenu menu;

  // Create close this tab action
  QAction *closeAction = new QAction(QString(), &menu);
  int index = tabWidget->tabBar()->tabAt(pos);
  if(index != -1 && tabWidget->count() > 1)
    // Enabled
    closeAction->setText(tr("&Close tab %1").arg(tabWidget->tabText(index).remove('&')));
  else
  {
    // Not over tab
    closeAction->setText(tr("Close tab"));
    closeAction->setDisabled(true);
  }
  closeAction->setToolTip(closeAction->text());
  closeAction->setStatusTip(closeAction->text());

  menu.addAction(actionAll);
  menu.addAction(actionNone);
  menu.addAction(actionReset);
  menu.addSeparator();
  menu.addAction(actionLock);
  menu.addSeparator();
  menu.addAction(closeAction);
  menu.addSeparator();

  for(const Tab& tab : tabs)
    menu.addAction(tab.action);

  // Open menu
  QAction *action = menu.exec(menuPos);
  if(action == closeAction)
    // Use internal which does not check locked status
    tabCloseRequestedInternal(index);
}

void TabWidgetHandler::init(const QVector<int>& tabIdsParam, const QString& settingsPrefixParam)
{
  clear();
  settingsPrefix = settingsPrefixParam;

  Q_ASSERT(tabIdsParam.size() == tabWidget->count());

  for(int index = 0; index < tabWidget->count(); index++)
  {
    QWidget *widget = tabWidget->widget(index);
    widget->setProperty(ID_PROPERTY, tabIdsParam.at(index));

    // Get id from array. This is doable because all tabs are expected to be loaded
    int id = tabIdsParam.at(index);

    // Get name with mnemonic
    QString name = tabWidget->tabText(index);
    if(!name.contains("&"))
      name = "&" + name;

    // Create and fill action
    QAction *action = new QAction(name, toolButtonCorner);
    action->setCheckable(true);
    action->setChecked(true);
    QString tooltip = tr("Open or close tab %1").arg(name).remove('&');
    action->setToolTip(tooltip);
    action->setStatusTip(tooltip);
    action->setData(id);
    toolButtonCorner->addAction(action);
    connect(action, &QAction::toggled, this, &TabWidgetHandler::toolbarActionTriggered);

    tabs.append(Tab(widget, tabWidget->tabText(index), tabWidget->tabToolTip(index), action));
  }
}

void TabWidgetHandler::restoreState()
{
  settings::Settings& settings = atools::settings::Settings::instance();

  // A list of tab ids in the same order as contained by the tab widget
  QStringList tabList = settings.valueStrList(settingsPrefix + "TabIds");

  if(!tabList.isEmpty())
  {
    clearTabWidget();
    QSignalBlocker blocker(tabWidget);

    for(const QString& str : tabList)
    {
      bool ok = false;
      int id = str.toInt(&ok);
      if(ok)
      {
        Tab tab = tabs.value(id);

        if(tab.isValid())
        {
          // Add tab to widget
          int idx = tabWidget->addTab(tab.widget, tab.title);
          if(idx != -1)
          {
            tabWidget->setTabToolTip(idx, tab.tooltip);
            tabWidget->setCurrentWidget(tab.widget);
          }
        }
      }
    }
  }

  // Restore current tab from separate setting
  setCurrentTab(settings.valueInt(settingsPrefix + "CurrentTabId", 0));

  {
    // Restore lock state
    QSignalBlocker blocker(actionLock);
    actionLock->setChecked(settings.valueBool(settingsPrefix + "Locked", false));
  }

  // Update actions
  updateTabs();
  updateWidgets();
}

void TabWidgetHandler::saveState()
{
  settings::Settings& settings = atools::settings::Settings::instance();

  // A list of tab ids in the same order as contained by the tab widget
  QStringList tabList;
  for(int index = 0; index < tabWidget->count(); index++)
    tabList.append(QString::number(idForIndex(index)));

  // Save tabs
  settings.setValue(settingsPrefix + "TabIds", tabList);

  // Save current tab
  settings.setValue(settingsPrefix + "CurrentTabId", getCurrentTabId());

  // Save lock state
  settings.setValue(settingsPrefix + "Locked", isLocked());
}

int TabWidgetHandler::getCurrentTabId() const
{
  return tabWidget->currentWidget() == nullptr ? -1 : tabWidget->currentWidget()->property(ID_PROPERTY).toInt();
}

void TabWidgetHandler::setCurrentTab(int id, bool left)
{
  int index = getIndexForId(id);
  if(index != -1)
    // Already open - push to front
    tabWidget->setCurrentIndex(index);
  else
  {
    {
      QSignalBlocker blocker(tabWidget);

      // Add and set active
      int idx = insertTab(tabWidget->currentIndex() + (left ? 0 : 1), id);
      if(idx != -1)
        tabWidget->setCurrentIndex(idx);
    }

    updateWidgets();
  }
}

void TabWidgetHandler::openTab(int id, bool left)
{
  int index = getIndexForId(id);
  if(index == -1)
  {
    {
      QSignalBlocker blocker(tabWidget);
      insertTab(tabWidget->currentIndex() + (left ? 0 : 1), id);
    }
    updateWidgets();
  }
}

bool TabWidgetHandler::isTabVisible(int id) const
{
  return getIndexForId(id) != -1;
}

void TabWidgetHandler::currentChanged()
{
  qDebug() << Q_FUNC_INFO << tabWidget->objectName();

  emit tabChanged(getCurrentTabId());
}

void TabWidgetHandler::tabCloseRequestedInternal(int index)
{
  qDebug() << Q_FUNC_INFO << tabWidget->objectName();

  if(tabWidget->count() > 1)
  {
    int id = idForIndex(index);
    tabWidget->removeTab(index);

    // Update action but disable signals to avoid recursion
    QAction *action = tabs.at(index).action;
    QSignalBlocker actionBlocker(action);
    action->setChecked(true);

    updateWidgets();
    emit tabClosed(id);
  }
}

void TabWidgetHandler::tabCloseRequested(int index)
{
  if(!isLocked())
    tabCloseRequestedInternal(index);
}

void TabWidgetHandler::toolbarActionTriggered()
{
  qDebug() << Q_FUNC_INFO << tabWidget->objectName();

  QAction *sendAction = dynamic_cast<QAction *>(sender());
  QWidget *current = tabWidget->currentWidget();

  {
    QSignalBlocker blocker(tabWidget);

    if(sendAction == actionAll)
    {
      // Add all closed tabs at the end of the list - keep current selected ==============================
      QVector<int> missing = missingTabIds();

      for(int id : missing)
        addTab(id);
      tabWidget->setCurrentWidget(current);
    }
    else if(sendAction == actionNone)
    {
      // Close all tabls except current one ============================================================
      clearTabWidget();
      addTab(idForWidget(current));
    }
    else if(sendAction == actionReset)
      // Reset open/close state and order and select first ============================================
      resetInternal();
    else if(sendAction == actionLock)
    {
      // Disallow movement and remove close buttons ============================================
      updateWidgets();
      updateTabs();
    }
    else if(sendAction != nullptr)
    {
      // Open/close individual tabs ============================================================
      int actionId = sendAction->data().toInt();

      if(sendAction->isChecked())
      {
        // Insert and activate
        int idx = insertTab(tabWidget->currentIndex() + 1, actionId);
        if(idx != -1)
          tabWidget->setCurrentIndex(idx);
      }
      else
      {
        int index = getIndexForId(actionId);
        if(index != -1)
          // Remove
          tabCloseRequestedInternal(index);
      }
    }
  }

  updateWidgets();
}

int TabWidgetHandler::insertTab(int index, int id)
{
  // Re-enables the close button
  fixSingleTab();

  // Add at index
  const Tab& tab = tabs.at(id);
  int idx = tabWidget->insertTab(index, tab.widget, tab.title);
  tabWidget->setTabToolTip(idx, tab.tooltip);
  tabWidget->setCurrentWidget(tab.widget);

  emit tabOpened(id);
  return idx;
}

int TabWidgetHandler::addTab(int id)
{
  // Re-enables the close button
  fixSingleTab();

  // Append to list
  const Tab& tab = tabs.at(id);
  int idx = tabWidget->addTab(tab.widget, tab.title);
  tabWidget->setTabToolTip(idx, tab.tooltip);

  emit tabOpened(id);
  return idx;
}

void TabWidgetHandler::fixSingleTab()
{
  if(tabWidget->count() == 1)
  {
    QWidget *tab = tabWidget->widget(0);
    QString title = tabWidget->tabText(0);
    QString tooltip = tabWidget->tabToolTip(0);
    tabWidget->removeTab(0);

    int idx = tabWidget->addTab(tab, title);
    tabWidget->setTabToolTip(idx, tooltip);
  }
}

void TabWidgetHandler::clearTabWidget()
{
  tabWidget->blockSignals(true);
  tabWidget->clear();
  tabWidget->blockSignals(false);
}

QVector<int> TabWidgetHandler::missingTabIds() const
{
  QVector<int> retval;

  for(int id = 0; id < tabs.size(); id++)
  {
    QWidget *widget = tabs.at(id).widget;
    int index = tabWidget->indexOf(widget);
    if(index == -1)
      retval.append(id);
  }

  return retval;
}

int TabWidgetHandler::idForIndex(int index) const
{
  QWidget *widget = tabWidget->widget(index);
  return widget != nullptr ? widget->property(ID_PROPERTY).toInt() : -1;
}

int TabWidgetHandler::idForWidget(QWidget *widget) const
{
  return widget->property(ID_PROPERTY).toInt();
}

int TabWidgetHandler::getIndexForId(int id) const
{
  return tabWidget->indexOf(tabs.at(id).widget);
}

bool TabWidgetHandler::isLocked() const
{
  return actionLock->isChecked();
}

void TabWidgetHandler::styleChanged()
{
  // workaround for macOS tabs which grow too big
#if defined(Q_OS_MACOS)

  QStyle *style = QApplication::style();
  if(style != nullptr)
  {
    QString name = style->objectName();
    if(name.contains("macintosh", Qt::CaseInsensitive) || name.contains("macos", Qt::CaseInsensitive))
      tabWidget->setElideMode(Qt::ElideRight);
    else
      tabWidget->setElideMode(Qt::ElideNone);
  }
  tabWidget->setUsesScrollButtons(true);
#endif
}

void TabWidgetHandler::updateTabs()
{
  tabWidget->setMovable(!isLocked());
  tabWidget->setTabsClosable(!isLocked());

  if(tabWidget->count() > 0)
    // Dummy text change to force a re-layout of the widget - otherwise tabs remain large
    tabWidget->setTabText(0, tabWidget->tabText(0));
}

void TabWidgetHandler::updateWidgets()
{
  for(const Tab& tab : tabs)
  {
    QSignalBlocker actionBlocker(tab.action);
    tab.action->setChecked(false);
    tab.action->setDisabled(false);
  }

  if(tabWidget->count() == 1)
  {
    tabWidget->tabBar()->setTabButton(0, QTabBar::RightSide, new QLabel(tabWidget));

    QAction *action = tabs.at(tabWidget->currentWidget()->property(ID_PROPERTY).toInt()).action;
    QSignalBlocker actionBlocker(action);
    action->setChecked(true);
    action->setDisabled(true);
  }
  else
  {
    for(int index = 0; index < tabWidget->count(); index++)
    {
      QAction *action = tabs.at(idForIndex(index)).action;
      QSignalBlocker actionBlocker(action);
      action->setChecked(true);
    }
  }
}

} // namespace gui
} // namespace atools
