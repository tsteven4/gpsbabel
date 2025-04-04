// -*- C++ -*-
// $Id: map.cpp,v 1.2 2009-08-28 17:08:55 robertl Exp $
//------------------------------------------------------------------------
//
//  Copyright (C) 2009  S. Khai Mong <khai@mangrai.com>.
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License as
//  published by the Free Software Foundation; either version 2 of the
//  License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
//  USA.
//
//------------------------------------------------------------------------
#include "map.h"

#include <QApplication>           // for QApplication
#include <QChar>                  // for QChar, operator!=
#include <QCursor>                // for QCursor
#include <QFile>                  // for QFile
#include <QIODevice>              // for QIODevice
#include <QLatin1String>          // for QLatin1String
#include <QList>                  // for QList
#include <QMessageBox>            // for QMessageBox
#include <QNetworkAccessManager>  // for QNetworkAccessManager
#include <QStringLiteral>         // for qMakeStringPrivate, QStringLiteral
#include <QUrl>                   // for QUrl
#include <QWebChannel>            // for QWebChannel
#include <QWebEnginePage>         // for QWebEnginePage
#include <QWebEngineSettings>     // for QWebEngineSettings
#include <QWebEngineView>         // for QWebEngineView
#include <Qt>                     // for CursorShape

#include <string>                 // for string
#include <vector>                 // for vector

#include "appname.h"              // for appName
#include "gpx.h"                  // for GpxRoute, GpxTrack, GpxWaypoint, Gpx, GpxRoutePoint, GpxTrackPoint, GpxTrackSegment
#include "latlng.h"               // for LatLng


using std::string;
using std::vector;

//------------------------------------------------------------------------
static QString stripDoubleQuotes(const QString& s)
{
  QString out = s;
  return out.remove('"');
}

//------------------------------------------------------------------------
Map::Map(QWidget* parent,
         const Gpx& gpx, QPlainTextEdit* te):
  QWebEngineView(parent),
  gpx_(gpx),
  textEdit_(te)
{
  busyCursor_ = true;
  stopWatch_.start();
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  manager_ = new QNetworkAccessManager(this);
  this->logTime("Start map constructor");

  auto* mclicker = new MarkerClicker(this);
  auto* channel = new QWebChannel(this->page());
  this->page()->setWebChannel(channel);
  // Note: A current limitation is that objects must be registered before any client is initialized.
  channel->registerObject(QStringLiteral("mclicker"), mclicker);
  connect(mclicker, &MarkerClicker::loadFinished, this, &Map::loadFinishedX);
  connect(mclicker, &MarkerClicker::markerClicked, this, &Map::markerClicked);
  connect(mclicker, &MarkerClicker::logTime, this, &Map::logTime);

  // We search the following locations:
  // 1. In the file system in the same directory as the executable.
  // 2. In the Qt resource system.  This is useful if the resource was compiled
  //    into the executable.
  QString baseFile = QApplication::applicationDirPath() + "/gmapbase.html";
  QString fileName;
  QUrl baseUrl;
  if (QFile(baseFile).exists()) {
    fileName = baseFile;
    baseUrl = QUrl::fromLocalFile(baseFile);
  } else if (QFile(":/gmapbase.html").exists()) {
    fileName = ":/gmapbase.html";
    baseUrl = QUrl("qrc:///gmapbase.html");
  }

  // If baseUrl is using the file scheme gmapbase.html will be local content,
  // and it needs to be able to access https://maps.googleapis.com, which is
  // remote.  Before 6.2.4 qrc was also considered a local scheme.
  // This changed with QTBUG-96849 in 6.2.4,
  // https://github.com/qt/qtwebengine/commit/dc7c2962a83a5eeb3c08e1a7312458ea5a18f4a5.
  // As of 6.2.4 if we don't set this flag we get net::ERR_NETWORK_ACCESS_DENIED with
  // the file scheme, and the map preview doesn't work.
  // In 6.2.3 we got a number of "from origin 'file://' has been blocked by CORS policy"
  // messages with the file scheme, but the map preview seemed to work.
  this->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);

  if (!fileName.isEmpty()) {
    QFile htmlFile(fileName);
    if (htmlFile.open(QIODevice::ReadOnly)) {
      QByteArray content = htmlFile.readAll();
      htmlFile.close();
      const QByteArray encodedKey = QByteArray::fromBase64("Qkp7YlR6Q3k6RFRiS2JQZWRENlRXM01uYltbQzdGUHlHbjJpTUk1");
      content.replace("APIKEY", decodeKey(encodedKey));
      this->setContent(content, "text/html;charset=UTF-8", baseUrl);
    } else {
      QMessageBox::critical(nullptr, appName,
                            tr("Error opening \"gmapbase.html\" file.  Check installation"));
    }
  } else {
    QMessageBox::critical(nullptr, appName,
                          tr("Missing \"gmapbase.html\" file.  Check installation"));
  }

#ifdef DEBUG_JS_GENERATION
  dbgdata_ = new QFile("mapdebug.js");
  if (dbgdata_->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    dbgout_ = new QTextStream(dbgdata_);
  }
#endif

}

//------------------------------------------------------------------------
QByteArray Map::encodeKey(const QByteArray& key)
{
  QByteArray rv;
  for (const auto c: key) {
    rv.append(c+1);
  }
  return rv;
}

//------------------------------------------------------------------------
QByteArray Map::decodeKey(const QByteArray& key)
{
  QByteArray rv;
  for (const auto c: key) {
    rv.append(c-1);
  }
  return rv;
}

//------------------------------------------------------------------------
Map::~Map()
{
  if (busyCursor_) {
    QApplication::restoreOverrideCursor();
  }
#ifdef DEBUG_JS_GENERATION
  if (dbgout_) {
    delete dbgout_;
    dbgout_ = nullptr;
  }
  if (dbgdata_) {
    delete dbgdata_;
    dbgdata_ = nullptr;
  }
#endif
}
//------------------------------------------------------------------------
void Map::loadFinishedX(bool f)
{
  this->logTime("Done initial page load");
  if (!f) {
    QMessageBox::critical(nullptr, appName,
                          tr("Failed to load Google maps base page"));
  } else {
    QApplication::processEvents();
    showGpxData();
  }
  QApplication::restoreOverrideCursor();
  busyCursor_ = false;
}

//------------------------------------------------------------------------
static QString fmtLatLng(const LatLng& l)
{
  return QString("{lat: %1, lng: %3}").arg(l.lat(), 0, 'f', 5) .arg(l.lng(), 0, 'f', 5);
}

//------------------------------------------------------------------------
static QString makePath(const vector <LatLng>& pts)
{
  // maps v3 Polylines do not use encoded paths.
  QString path;
  int lncount = 0;
  bool someoutput = false;
  for (const auto& ll : pts) {
    if (lncount == 0) {
      if (someoutput) {
        path.append(QChar(','));
      }
      path.append(QLatin1String("\n            "));
    } else if (lncount == 1) {
      path.append(QLatin1String(", "));
    }
    path.append(fmtLatLng(ll));
    someoutput = true;
    lncount = (lncount + 1) % 2;
  }
  return path;
}

//------------------------------------------------------------------------
void Map::showGpxData()
{
  this->logTime("Start defining JS string");
  QStringList scriptStr;
  scriptStr
      << "mclicker.logTimeX(\"Start JS execution\");"
      << "var map = new google.maps.Map(document.getElementById(\"map\"));"
      << "var bounds = new google.maps.LatLngBounds();"
      << "var waypts = [];"
      << "var rtes = [];"
      << "var trks = [];"
      << "var idx;"
      << "mclicker.logTimeX(\"Done prelim JS definition\");"
      ;

  mapPresent_ = true;

  // Waypoints.
  int num=0;
  for (const GpxWaypoint& pt : gpx_.getWaypoints()) {
    scriptStr
        << QString("waypts[%1] = new google.maps.Marker({map: map, position: %2, "
                   "title: \"%3\", icon: blueIcon});")
        .arg(num)
        .arg(fmtLatLng(pt.getLocation()), stripDoubleQuotes(pt.getName()));
    num++;
  }

  scriptStr
      << "for (idx = 0; idx < waypts.length; idx += 1) {"
      << "    bounds.extend(waypts[idx].getPosition());"
      << "    attachHandler(waypts[idx], new MarkerHandler(0, idx));"
      << "}"
      << "mclicker.logTimeX(\"Done waypoints definition\");"
      ;

  // Tracks
  num = 0;
  for (const GpxTrack& trk : gpx_.getTracks()) {
    vector <LatLng> pts;
    for (const GpxTrackSegment& seg : trk.getTrackSegments()) {
      for (const GpxTrackPoint& pt : seg.getTrackPoints()) {
        pts.push_back(pt.getLocation());
      }
    }
    QString path = makePath(pts);

    scriptStr
        << QString("trks[%1] = new RTPolyline(\n"
                   "    map,\n"
                   "    new google.maps.Polyline({\n        map: map,\n        strokeColor: \"#0000E0\",\n        strokeWeight: 2,\n        strokeOpacity: 0.6,\n        path: [%2\n        ]\n    }),\n"
                   "    new google.maps.LatLng(%3),\n"
                   "    new google.maps.LatLng(%4),\n"
                   "    \"%5\",\n"
                   "    new MarkerHandler(1, %1)\n);"
                  ).arg(num).arg(path, fmtLatLng(pts.front()), fmtLatLng(pts.back()), stripDoubleQuotes(trk.getName()))
        << QString("bounds.union(trks[%1].getBounds());").arg(num)
        ;
    num++;
  }

  scriptStr
      << "mclicker.logTimeX(\"Done track definition\");"
      ;

  // Routes
  num = 0;
  for (const GpxRoute& rte : gpx_.getRoutes()) {
    vector <LatLng> pts;
    for (const GpxRoutePoint& pt : rte.getRoutePoints()) {
      pts.push_back(pt.getLocation());
    }
    QString path = makePath(pts);

    scriptStr
        << QString("rtes[%1] = new RTPolyline(\n"
                   "    map,\n"
                   "    new google.maps.Polyline({\n        map: map,\n        strokeColor: \"#8000B0\",\n        strokeWeight: 2,\n        strokeOpacity: 0.6,\n        path: [%2\n        ]\n    }),\n"
                   "    new google.maps.LatLng(%3),\n"
                   "    new google.maps.LatLng(%4),\n"
                   "    \"%5\",\n"
                   "    new MarkerHandler(2, %1)\n);"
                  ).arg(num).arg(path, fmtLatLng(pts.front()), fmtLatLng(pts.back()), stripDoubleQuotes(rte.getName()))
        << QString("bounds.union(rtes[%1].getBounds());").arg(num)
        ;
    num++;
  }

  scriptStr
      << "mclicker.logTimeX(\"Done route definition\");"
      ;

  scriptStr
      << "map.setCenter(bounds.getCenter());"
      << "map.fitBounds(bounds);"
      << "mclicker.logTimeX(\"Done setCenter\");"
      ;

  this->logTime("Done defining JS string");
  evaluateJS(scriptStr);
  this->logTime("Done JS evaluation");
}

//------------------------------------------------------------------------
void Map::markerClicked(int t, int i)
{
  if (t == 0) {
    emit waypointClicked(i);
  } else if (t == 1) {
    emit trackClicked(i);
  } else if (t == 2) {
    emit routeClicked(i);
  }
}

//------------------------------------------------------------------------
void Map::logTime(const QString& s)
{
  //  fprintf(stderr, "Log: %s:  %d ms\n", s.toStdString().c_str(), stopWatch.elapsed());
  if (textEdit_ != nullptr) {
    textEdit_->appendPlainText(QString("%1: %2 ms").arg(s).arg(stopWatch_.elapsed()));
  }
  stopWatch_.start();
}
//------------------------------------------------------------------------
void Map::setWaypointVisibility(int i, bool show)
{
  evaluateJS(QString("waypts[%1].setVisible(%2);")
             .arg(i).arg(show?"true": "false"));
}

//------------------------------------------------------------------------
void Map::setTrackVisibility(int i, bool show)
{
  evaluateJS(QString("trks[%1].%2();").arg(i).arg(show?"show": "hide"));
}

//------------------------------------------------------------------------
void Map::setRouteVisibility(int i, bool show)
{
  evaluateJS(QString("rtes[%1].%2();").arg(i).arg(show?"show": "hide"));
}

//------------------------------------------------------------------------
void Map::resetBounds()
{
  evaluateJS(QStringList{
    "map.setCenter(bounds.getCenter());",
    "map.fitBounds(bounds);",
  });
}

//------------------------------------------------------------------------
void Map::panTo(const LatLng& loc)
{
  evaluateJS(QString("map.panTo(new google.maps.LatLng(%1));").arg(fmtLatLng(loc)));
}

//------------------------------------------------------------------------
void Map::resizeEvent(QResizeEvent* ev)
{
  QWebEngineView::resizeEvent(ev);
  if (mapPresent_) {
    evaluateJS(QString("google.maps.event.trigger(map, 'resize');"));
  }
}

//------------------------------------------------------------------------
void Map::setWaypointColorRed(int i)
{
  evaluateJS(QString("waypts[%1].setIcon(redIcon);").arg(i));
}

//------------------------------------------------------------------------
void Map::setWaypointColorBlue(int i)
{
  evaluateJS(QString("waypts[%1].setIcon(blueIcon);").arg(i));
}

//------------------------------------------------------------------------
void Map::frameTrack(int i)
{
  evaluateJS(QStringList{
    QString("map.setCenter(trks[%1].getBounds().getCenter());").arg(i),
    QString("map.fitBounds(trks[%1].getBounds());").arg(i),
  });
}

//------------------------------------------------------------------------
void Map::frameRoute(int i)
{
  evaluateJS(QStringList{
    QString("map.setCenter(rtes[%1].getBounds().getCenter());").arg(i),
    QString("map.fitBounds(rtes[%1].getBounds());").arg(i),
  });
}

//------------------------------------------------------------------------
void Map::evaluateJS(const QString& s, bool upd)
{
#ifdef DEBUG_JS_GENERATION
  *dbgout_ << s << '\n';
  dbgout_->flush();
#endif
  this->page()->runJavaScript(s);
  if (upd) {
    this->update();
  }
}

//------------------------------------------------------------------------
void Map::evaluateJS(const QStringList& s, bool upd)
{
  evaluateJS(s.join('\n'), upd);
}
