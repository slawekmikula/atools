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

#include "fs/scenery/contentxml.h"

#include "util/xmlstream.h"
#include "exception.h"
#include "fs/scenery/sceneryarea.h"

#include <QFile>
#include <QDir>
#include <QDebug>

namespace atools {
namespace fs {
namespace scenery {

/*
 *  <Content>
 *  <Package name="fs-base" active="true"/>
 *  <Package name="asobo-airport-kord-chicago-ohare" active="true"/>
 *  ...
 *  <Package name="asobo-airport-vqpr-paro" active="true"/>
 *  <Package name="asobo-liveevent" active="true"/>
 *  <Package name="asobo-airport-nzqn-queenstown" active="true"/>
 *  <Package name="asobo-airport-sbgl-riodejaneiro" active="true"/>
 *  <Package name="fs-base-nav" active="true"/>
 *  <Package name="asobo-modellib-props" active="true"/>
 *  <Package name="asobo-modellib-airport-generic" active="true"/>
 *  <Package name="asobo-modellib-buildings" active="true"/>
 *  <Package name="asobo-cameras" active="true"/>
 *  <Package name="asobo-jetways" active="true"/>
 *  <Package name="fs-base-ai-traffic" active="true"/>
 *  </Content>
 */
void ContentXml::read(const QString& filename)
{
  areaEntries.clear();
  number = 5;

  QFile xmlFile(filename);
  if(xmlFile.open(QIODevice::ReadOnly))
  {
    atools::util::XmlStream xmlStream(&xmlFile, filename);
    QXmlStreamReader& reader = xmlStream.getReader();

    xmlStream.readUntilElement("Content");

    while(xmlStream.readNextStartElement())
    {
      if(reader.name() == "Package")
      {
        QString name = reader.attributes().value("name").toString();

        int num;
        QString title;
        if(name == "fs-base")
        {
          num = 0;
          title = tr("Base Airports");
        }
        else if(name == "fs-base-nav")
        {
          num = 1;
          title = tr("Base Navigation");
        }
        else
          num = number++;

        if(name == "fs-base" || name == "fs-base-nav")
        {
          SceneryArea area(num, num, title, name);
          area.setActive(reader.attributes().value("active").toString().toLower() == "true");
          areaEntries.append(area);
          number++;
        }

        // Read only attributes
        xmlStream.skipCurrentElement();
      }
      else
        xmlStream.skipCurrentElement(true /* warn */);
    }
    xmlFile.close();
  }
  else
    throw atools::Exception(tr("Cannot open file \"%1\". Reason: %2").arg(filename).arg(xmlFile.errorString()));
}

void ContentXml::fillDefault()
{
  SceneryArea area(0, 0, tr("Base Airports"), "fs-base");
  area.setActive(true);
  areaEntries.append(area);

  SceneryArea areaNav(1, 1, tr("Base Navigation"), "fs-base-nav");
  areaNav.setActive(true);
  areaEntries.append(areaNav);
}

QDebug operator<<(QDebug out, const ContentXml& cfg)
{
  QDebugStateSaver saver(out);

  out.nospace() << "ContentXml[" << endl;

  for(const SceneryArea& area : cfg.areaEntries)
    out.nospace().noquote() << area << endl;

  out.nospace().noquote() << endl << "]";
  return out;
}

} // namespace scenery
} // namespace fs
} // namespace atools
