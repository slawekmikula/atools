/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_FGCONNECTHANDLER_H
#define ATOOLS_FGCONNECTHANDLER_H

#include "fs/sc/connecthandler.h"

#include <QObject>
#include <QUdpSocket>
#include <functional>

namespace atools {
namespace fs {
namespace sc {

/*
 * Reads data from a callback function into SimConnectData.
 */
class FgConnectHandler :
  QObject,
  public atools::fs::sc::ConnectHandler
{
    Q_OBJECT
public:
  FgConnectHandler(QObject *parent);
  virtual ~FgConnectHandler() override;

  virtual bool connect() override;

  virtual bool isLoaded() const override;

  virtual bool fetchData(SimConnectData& data, int radiusKm, Options options) override;

  virtual bool fetchWeatherData(SimConnectData& data) override;

  virtual void addWeatherRequest(const WeatherRequest& request) override;

  virtual const WeatherRequest& getWeatherRequest() const override;

  virtual bool isSimRunning() const override;

  virtual bool isSimPaused() const override;

  virtual atools::fs::sc::State getState() const override;

  QString getName() const override;

private slots:
  void readPendingDatagrams();

private:
  void disconnect();  

  QUdpSocket* udpSocket = nullptr;
  atools::fs::sc::State state = DISCONNECTED;
};

} // namespace sc
} // namespace fs
} // namespace atools

#endif // ATOOLS_FGCONNECTHANDLER_H
