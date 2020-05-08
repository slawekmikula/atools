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

#include "fs/sc/simconnectdata.h"

#include "geo/calculations.h"

#include <QDebug>
#include <QDataStream>

using atools::fs::weather::MetarResult;

namespace atools {
namespace fs {
namespace sc {

SimConnectData::SimConnectData()
{

}

SimConnectData::SimConnectData(const SimConnectData& other)
{
  *this = other;
}

SimConnectData::~SimConnectData()
{

}

bool SimConnectData::read(QIODevice *ioDevice)
{
  status = OK;

  QDataStream in(ioDevice);
  in.setVersion(QDataStream::Qt_5_5);
  in.setFloatingPointPrecision(QDataStream::SinglePrecision);

  if(magicNumber == 0)
  {
    if(ioDevice->bytesAvailable() < static_cast<qint64>(sizeof(magicNumber)))
      return false;

    in >> magicNumber;
    if(magicNumber != MAGIC_NUMBER_DATA)
    {
      qWarning() << "SimConnectData::read: invalid magic number" << magicNumber;
      status = INVALID_MAGIC_NUMBER;
      return false;
    }
  }

  if(packetSize == 0)
  {
    if(ioDevice->bytesAvailable() < static_cast<qint64>(sizeof(packetSize)))
      return false;

    in >> packetSize;
  }

  // Wait until the whole packet is available
  if(ioDevice->bytesAvailable() < packetSize)
    return false;

  in >> version;
  if(version != DATA_VERSION)
  {
    qWarning() << "SimConnectData::read: version mismatch" << version << "!=" << DATA_VERSION;
    status = VERSION_MISMATCH;
    return false;
  }
  in >> packetId >> packetTs;

  quint8 hasUser = 0;
  in >> hasUser;
  if(hasUser == 1)
    userAircraft.read(in);

  quint16 numAi = 0;
  in >> numAi;
  for(quint16 i = 0; i < numAi; i++)
  {
    SimConnectAircraft ap;
    ap.read(in);
    aiAircraft.append(ap);
  }

  quint16 numMetar = 0;
  in >> numMetar;
  for(quint16 i = 0; i < numMetar; i++)
  {

    MetarResult result;
    readString(in, result.requestIdent);

    float lonx, laty, altitude;
    quint32 minSinceEpoch;
    in >> lonx >> laty >> altitude >> minSinceEpoch;
    result.requestPos.setAltitude(altitude);
    result.requestPos.setLonX(lonx);
    result.requestPos.setLatY(laty);
    result.timestamp = QDateTime::fromMSecsSinceEpoch(minSinceEpoch * 1000);

    readLongString(in, result.metarForStation);
    readLongString(in, result.metarForNearest);
    readLongString(in, result.metarForInterpolated);

    metarResults.append(result);
  }

  return true;
}

int SimConnectData::write(QIODevice *ioDevice)
{
  status = OK;

  QByteArray block;
  QDataStream out(&block, QIODevice::WriteOnly);
  out.setVersion(QDataStream::Qt_5_5);
  out.setFloatingPointPrecision(QDataStream::SinglePrecision);

  out << MAGIC_NUMBER_DATA << packetSize << DATA_VERSION << packetId << packetTs;

  bool userValid = userAircraft.getPosition().isValid();
  out << static_cast<quint8>(userValid);
  if(userValid)
    userAircraft.write(out);

  int numAi = std::min(65535, aiAircraft.size());
  out << static_cast<quint16>(numAi);

  for(int i = 0; i < numAi; i++)
    aiAircraft.at(i).write(out);

  int numMetar = std::min(65535, metarResults.size());
  out << static_cast<quint16>(numMetar);

  for(int i = 0; i < numMetar; i++)
  {
    const MetarResult& result = metarResults.at(i);
    writeString(out, result.requestIdent);
    out << result.requestPos.getLonX() << result.requestPos.getLatY() << result.requestPos.getAltitude()
        << static_cast<quint32>(result.timestamp.currentMSecsSinceEpoch() / 1000);
    writeLongString(out, result.metarForStation);
    writeLongString(out, result.metarForNearest);
    writeLongString(out, result.metarForInterpolated);
  }

  // Go back and update size
  out.device()->seek(sizeof(MAGIC_NUMBER_DATA));
  int size = block.size() - static_cast<int>(sizeof(packetSize)) - static_cast<int>(sizeof(MAGIC_NUMBER_DATA));
  out << static_cast<quint32>(size);

  return SimConnectDataBase::writeBlock(ioDevice, block, status);
}

SimConnectData SimConnectData::buildDebugForPosition(const geo::Pos& pos, const geo::Pos& lastPos, bool ground,
                                                     float vertSpeed, float tas, float fuelflow, float totalFuel,
                                                     float ice)
{
  static QVector<float> lastHdgs;
  lastHdgs.fill(0.f, 10);

  SimConnectData data;
  data.userAircraft.position = pos;
  // data.userAircraft.position.setAltitude(1000);

  float h = 0.f;
  if(lastPos.isValid())
  {
    h = !lastPos.almostEqual(pos, atools::geo::Pos::POS_EPSILON_10M) ? lastPos.angleDegTo(pos) : 0.f;
    data.userAircraft.groundSpeedKts = data.userAircraft.indicatedSpeedKts = data.userAircraft.trueAirspeedKts = tas;
  }

  data.userAircraft.trackMagDeg = atools::geo::normalizeCourse(h + 20.f);
  data.userAircraft.trackTrueDeg = atools::geo::normalizeCourse(h + 25.f);
  data.userAircraft.headingMagDeg = atools::geo::normalizeCourse(h + 10.f);
  data.userAircraft.headingTrueDeg = atools::geo::normalizeCourse(h + 15.f);

  data.userAircraft.structuralIcePercent = ice;
  data.userAircraft.category = AIRPLANE;
  data.userAircraft.engineType = PISTON;
  data.userAircraft.zuluDateTime = QDateTime::currentDateTimeUtc();
  data.userAircraft.localDateTime = QDateTime::currentDateTime();

  data.userAircraft.airplaneTitle = "Airplane Title";
  data.userAircraft.airplaneType = "Airplane Type";
  data.userAircraft.airplaneModel = "MODEL";
  data.userAircraft.airplaneReg = "Airplane Registration";
  data.userAircraft.airplaneAirline = "Airline";
  data.userAircraft.airplaneFlightnumber = "965";
  data.userAircraft.fromIdent = "EDDF";

  data.userAircraft.verticalSpeedFeetPerMin = vertSpeed;

  data.userAircraft.toIdent = "LIRF";
  data.userAircraft.altitudeAboveGroundFt = pos.getAltitude();
  data.userAircraft.indicatedAltitudeFt = pos.getAltitude();
  data.userAircraft.airplaneEmptyWeightLbs = 1500.f;
  data.userAircraft.airplaneTotalWeightLbs = 3000.f;
  data.userAircraft.airplaneMaxGrossWeightLbs = 4000.f;
  data.userAircraft.fuelTotalWeightLbs = totalFuel;
  data.userAircraft.fuelTotalQuantityGallons = atools::geo::fromLbsToGal(false, data.userAircraft.fuelTotalWeightLbs);
  data.userAircraft.fuelFlowPPH = fuelflow;
  data.userAircraft.fuelFlowGPH = atools::geo::fromLbsToGal(false, fuelflow);
  data.userAircraft.flags = IS_USER | (ground ? ON_GROUND : NONE);

  data.userAircraft.debug = true;

  return data;
}

} // namespace sc
} // namespace fs
} // namespace atools
