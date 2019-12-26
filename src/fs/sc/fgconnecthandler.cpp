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

#include "fs/sc/fgconnecthandler.h"
#include "fs/sc/weatherrequest.h"
#include "fs/sc/simconnectdata.h"

#include <QBuffer>
#include <QDataStream>
#include <QNetworkDatagram>

using atools::geo::Pos;

namespace atools {
namespace fs {
namespace sc {

FgConnectHandler::FgConnectHandler(QObject *parent)
    : QObject(parent)
{
  qDebug() << Q_FUNC_INFO;
}

FgConnectHandler::~FgConnectHandler()
{
  qDebug() << Q_FUNC_INFO;
  disconnect();
}

bool FgConnectHandler::connect()
{
  if(udpSocket != nullptr) {
      if (udpSocket->state() == udpSocket->BoundState) {
        qDebug() << Q_FUNC_INFO << "Already open";
        state = STATEOK;
        return true;
      }
  }

  udpSocket = new QUdpSocket(this->parent());
  if (!udpSocket->bind(7755, QUdpSocket::ShareAddress)) {
    qWarning() << Q_FUNC_INFO << "Cannot open UDP port";
    state = OPEN_ERROR;
    udpSocket = nullptr;
    return false;
  }
  else
  {    
    QObject::connect(udpSocket, SIGNAL(readyRead()), this, SLOT(readPendingDatagrams()));
    qInfo() << Q_FUNC_INFO << "Attached to the UDP port";
    state = STATEOK;
    return true;
  }
}

void FgConnectHandler::disconnect()
{
  if(udpSocket != nullptr) {
    if (udpSocket->state() == udpSocket->BoundState) {
      qDebug() << Q_FUNC_INFO << "Closing connection";
      udpSocket->close();
      delete udpSocket;
    }
  }

  qDebug() << Q_FUNC_INFO << "closed";
  state = DISCONNECTED;
}

void FgConnectHandler::readPendingDatagrams()
{
    QByteArray rxData;
    QHostAddress sender;
    quint16 senderPort;

    while (udpSocket->hasPendingDatagrams())
    {
        qInfo() << Q_FUNC_INFO << "Read pending datagrams";

        // Resize and zero byte buffer so we can make way for the new data.
        rxData.fill(0, udpSocket->pendingDatagramSize());

        // Read data from the UDP buffer.
        udpSocket->readDatagram(rxData.data(),
                                rxData.size(),
                                &sender,
                                &senderPort);

        qInfo() << Q_FUNC_INFO << "Received: " << rxData.size();
    }
}

bool FgConnectHandler::fetchData(fs::sc::SimConnectData& data, int radiusKm, fs::sc::Options options)
{
  Q_UNUSED(data)
  Q_UNUSED(radiusKm)
  Q_UNUSED(options)

  if(udpSocket == nullptr)
  {
    state = DISCONNECTED;
    return false;
  }

  QString retval = "2019-12-26T09:27:49;7200;3.564416;763.517456;2.513103;150.000000;26.924566;30.476448;39.534885;238.000000;5.453534;28949.210938;354.546478;347.994141;Piper PA28-161 Warrior II (160hp);PA28-161-160;50.0742416382;19.8020534515;347.994141;342.540619;0.000000;229.615570;0.004633;0.004734;0.000007;-12.254924";
  QStringList pieces = retval.split(";");

  if (pieces.size() == 1) {
      return false;
  }

  int index = 0;
  //      <name>2019-12-25T09:43:49</name>
  QString timegmt = pieces.at(index++);
  //      <name>7200</name>
  int timeLocalOffset = pieces.at(index++).toInt();
  //      <name>altitudeAboveGroundFt = 0.f</name>
  float altitudeAboveGroundFt = pieces.at(index++).toFloat();
  //      <name>groundAltitudeFt = 0.f</name>
  float groundAltitudeFt = pieces.at(index++).toFloat();
  //      <name>windSpeedKts = 0.f</name>
  float windSpeedKts = pieces.at(index++).toFloat();
  //      <name>windDirectionDegT = 0.f</name>
  float windDirectionDegT = pieces.at(index++).toFloat();
  //      <name>ambientTemperatureCelsius = 0.f</name>
  float ambientTemperatureCelsius = pieces.at(index++).toFloat();
  //      <name>seaLevelPressureMbar = 0.f - inhg</name>
  float seaLevelPressureInhg = pieces.at(index++).toFloat();
  //      <name>fuelTotalQuantityGallons = 0.f</name>
  float fuelTotalQuantityGallons = pieces.at(index++).toFloat();
  //      <name>fuelTotalWeightLbs = 0.f</name>
  float fuelTotalWeightLbs = pieces.at(index++).toFloat();
  //      <name>magVarDeg = 0.f</name>
  float magVarDeg = pieces.at(index++).toFloat();
  //      <name>ambientVisibilityMeter = 0.f;f</name>
  float ambientVisibilityMeter = pieces.at(index++).toFloat();
  //      <name>trackMagDeg = 0.f;</name>
  float trackMagDeg = pieces.at(index++).toFloat();
  //      <name>trackTrueDeg = 0.f;</name>
  float trackTrueDeg = pieces.at(index++).toFloat();
  //      <name>airplaneTitle = ""</name>
  QString airplaneTitle = pieces.at(index++);
  //      <name>airplaneModel = ""</name>
  QString airplaneModel = pieces.at(index++);
  //      <name>atools::geo::Pos position - latitude</name>
  float latitude = pieces.at(index++).toFloat();
  //      <name>atools::geo::Pos position - longitude</name>
  float longitude = pieces.at(index++).toFloat();
  //      <name>headingTrueDeg = 0.f</name>
  float headingTrueDeg = pieces.at(index++).toFloat();
  //      <name>headingMagDeg = 0.f</name>
  float headingMagDeg = pieces.at(index++).toFloat();
  //      <name>groundSpeedKts = 0.f</name>
  float groundSpeedKts = pieces.at(index++).toFloat();
  //      <name>indicatedAltitudeFt = 0.f</name>
  float indicatedAltitudeFt = pieces.at(index++).toFloat();
  //      <name>indicatedSpeedKts = 0.f</name>
  float indicatedSpeedKts = pieces.at(index++).toFloat();
  //      <name>trueAirspeedKts = 0.f</name>
  float trueAirspeedKts = pieces.at(index++).toFloat();
  //      <name>machSpeed = 0.f</name>
  float machSpeed = pieces.at(index++).toFloat();
  //      <name>verticalSpeedFeetPerMin = 0.f</name>
  float verticalSpeedFeetPerMin = pieces.at(index++).toFloat();

  atools::fs::sc::SimConnectUserAircraft& userAircraft = data.userAircraft;

  userAircraft.position = Pos(longitude, latitude, altitudeAboveGroundFt);
  if(!userAircraft.position.isValid() || userAircraft.position.isNull()) {
    return false;
  }

  // Build local time
  QDateTime zuluDateTime = QDateTime::fromString(timegmt, "yyyy-MM-ddTHH:mm:ss");
  userAircraft.zuluDateTime = zuluDateTime;
  QDateTime localDateTime(zuluDateTime.date(), zuluDateTime.time(), Qt::OffsetFromUTC, timeLocalOffset);
  userAircraft.localDateTime = localDateTime;

  userAircraft.magVarDeg = magVarDeg;

  // Wind and ambient parameters
  userAircraft.windSpeedKts = windSpeedKts;
  userAircraft.windDirectionDegT = windDirectionDegT;
  userAircraft.ambientTemperatureCelsius = ambientTemperatureCelsius;
  userAircraft.totalAirTemperatureCelsius = ambientTemperatureCelsius; // FIXME
  userAircraft.seaLevelPressureMbar = seaLevelPressureInhg/0.029530f;

  // Ice
  // userAircraft.pitotIcePercent
  // userAircraft.structuralIcePercent

  // Weight
  // userAircraft.airplaneTotalWeightLbs
  // userAircraft.airplaneMaxGrossWeightLbs
  // userAircraft.airplaneEmptyWeightLbs

  // Fuel flow in weight
  userAircraft.fuelTotalWeightLbs = fuelTotalWeightLbs;
  userAircraft.fuelTotalQuantityGallons = fuelTotalQuantityGallons;
  // userAircraft.fuelFlowPPH
  // userAircraft.fuelFlowGPH
  // userAircraft.numberOfEngines

  userAircraft.ambientVisibilityMeter = ambientVisibilityMeter;

  // SimConnectAircraft
  userAircraft.airplaneTitle = airplaneTitle;
  userAircraft.airplaneModel = airplaneModel;
  // userAircraft.airplaneReg;
  // userAircraft.airplaneType;
  // userAircraft.airplaneAirline;
  // userAircraft.airplaneFlightnumber;
  // userAircraft.fromIdent;
  // userAircraft.toIdent;

  userAircraft.altitudeAboveGroundFt = altitudeAboveGroundFt;
  userAircraft.groundAltitudeFt = groundAltitudeFt;
  userAircraft.indicatedAltitudeFt = indicatedAltitudeFt;

  // Heading and track
  userAircraft.headingMagDeg = headingMagDeg;
  userAircraft.headingTrueDeg = headingTrueDeg;
  userAircraft.trackMagDeg = trackMagDeg;
  userAircraft.trackTrueDeg = trackTrueDeg;

  // Speed
  userAircraft.indicatedSpeedKts = indicatedSpeedKts;
  userAircraft.trueAirspeedKts = trueAirspeedKts;
  userAircraft.machSpeed = machSpeed;
  userAircraft.verticalSpeedFeetPerMin = verticalSpeedFeetPerMin;
  userAircraft.groundSpeedKts = groundSpeedKts;

  // Model
  // userAircraft.modelRadiusFt
  // userAircraft.wingSpanFt

  // Set misc flags
  userAircraft.flags = atools::fs::sc::IS_USER | atools::fs::sc::SIM_XPLANE; // FIXME
  if((int)altitudeAboveGroundFt > 0) {
    userAircraft.flags |= atools::fs::sc::ON_GROUND;
  }

  // userAircraft.flags |= atools::fs::sc::IN_RAIN;
  // IN_CLOUD = 0x0002, - not available
  // IN_SNOW = 0x0008,  - not available

  // userAircraft.flags |= atools::fs::sc::SIM_PAUSED;
  // userAircraft.flags |= atools::fs::sc::SIM_REPLAY;

  userAircraft.category = atools::fs::sc::UNKNOWN;
  // AIRPLANE, HELICOPTER, BOAT, GROUNDVEHICLE, CONTROLTOWER, SIMPLEOBJECT, VIEWER

  userAircraft.engineType = atools::fs::sc::UNSUPPORTED;
  // PISTON = 0, JET = 1, NO_ENGINE = 2, HELO_TURBINE = 3, UNSUPPORTED = 4, TURBOPROP = 5

  return true;
}

bool FgConnectHandler::fetchWeatherData(fs::sc::SimConnectData& data)
{
  Q_UNUSED(data)
  return false;
}

void FgConnectHandler::addWeatherRequest(const fs::sc::WeatherRequest& request)
{
  Q_UNUSED(request)
}

const atools::fs::sc::WeatherRequest& FgConnectHandler::getWeatherRequest() const
{
  static atools::fs::sc::WeatherRequest dummy;
  return dummy;
}

bool FgConnectHandler::isSimRunning() const
{
  return state == STATEOK;
}

bool FgConnectHandler::isSimPaused() const
{
  return false;
}

atools::fs::sc::State FgConnectHandler::getState() const
{
  return state;
}

QString FgConnectHandler::getName() const
{
  return QLatin1Literal("FgConnect");
}

bool FgConnectHandler::isLoaded() const
{
  return true;
}

} // namespace sc
} // namespace fs
} // namespace atools
